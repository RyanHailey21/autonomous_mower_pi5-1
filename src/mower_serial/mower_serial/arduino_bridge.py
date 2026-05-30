import time

import serial

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node
from std_msgs.msg import String


class ArduinoBridge(Node):
    def __init__(self):
        super().__init__('arduino_bridge')

        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('wheel_separation_m', 0.52)
        self.declare_parameter('max_linear_mps', 0.5)
        self.declare_parameter('max_pwm', 60)
        self.declare_parameter('cmd_vel_timeout_s', 0.5)

        port = self.get_parameter('port').value
        baud = self.get_parameter('baud').value
        self.wheel_separation_m = self.get_parameter('wheel_separation_m').value
        self.max_linear_mps = self.get_parameter('max_linear_mps').value
        self.max_pwm = self.get_parameter('max_pwm').value
        self.cmd_vel_timeout_s = self.get_parameter('cmd_vel_timeout_s').value
        if self.max_linear_mps <= 0.0:
            self.get_logger().warn('max_linear_mps must be positive; using 0.5')
            self.max_linear_mps = 0.5
        if self.max_pwm < 0:
            self.get_logger().warn('max_pwm must be nonnegative; using 0')
            self.max_pwm = 0
        self.last_cmd_vel_time = None
        self.last_drive_command = (0, 0)

        self.get_logger().info(f'Opening serial port {port} at {baud} baud')
        self.serial = serial.Serial(port, baud, timeout=0)
        time.sleep(2)
        self._serial_rx_buffer = bytearray()

        self.subscription = self.create_subscription(
            String,
            '/mower/arduino_cmd',
            self.command_callback,
            10
        )
        self.cmd_vel_subscription = self.create_subscription(
            Twist,
            '/cmd_vel',
            self.cmd_vel_callback,
            10
        )
        self.serial_timer = self.create_timer(0.05, self.read_serial)
        self.heartbeat_timer = self.create_timer(0.5, self.send_heartbeat)
        self.cmd_vel_watchdog_timer = self.create_timer(0.1, self.check_cmd_vel_timeout)
        self.send_heartbeat()

        self.get_logger().info('Ready. Publish Twist commands to /cmd_vel')
        self.get_logger().info('Debug commands are still available on /mower/arduino_cmd')

    def command_callback(self, msg):
        command = msg.data.strip()
        if not command:
            return

        self.write_command(command)
        self.get_logger().info(f'Sent to Arduino: {command}')

    def write_command(self, command):
        self.serial.write((command + '\n').encode('utf-8'))

    def send_heartbeat(self):
        self.write_command('BRIDGE_HEARTBEAT')

    def cmd_vel_callback(self, msg):
        linear = msg.linear.x
        angular = msg.angular.z

        left_mps = linear - angular * self.wheel_separation_m / 2.0
        right_mps = linear + angular * self.wheel_separation_m / 2.0

        left_pwm = self.speed_to_pwm(left_mps)
        right_pwm = self.speed_to_pwm(right_mps)

        self.last_cmd_vel_time = self.get_clock().now()
        self.send_drive_command(left_pwm, right_pwm)

    def speed_to_pwm(self, speed_mps):
        if speed_mps <= 0.0:
            return 0

        scaled = round((speed_mps / self.max_linear_mps) * self.max_pwm)
        return max(0, min(self.max_pwm, scaled))

    def send_drive_command(self, left_pwm, right_pwm):
        command = (left_pwm, right_pwm)
        if command == self.last_drive_command:
            return

        self.write_command(f'DRIVE {left_pwm} {right_pwm}')
        self.last_drive_command = command
        self.get_logger().info(f'Sent drive PWM: left={left_pwm} right={right_pwm}')

    def check_cmd_vel_timeout(self):
        if self.last_cmd_vel_time is None:
            return

        age = self.get_clock().now() - self.last_cmd_vel_time
        if age.nanoseconds / 1e9 > self.cmd_vel_timeout_s:
            self.send_drive_command(0, 0)
            self.last_cmd_vel_time = None

    def read_serial(self):
        waiting = self.serial.in_waiting
        if not waiting:
            return

        self._serial_rx_buffer.extend(self.serial.read(waiting))

        while b'\n' in self._serial_rx_buffer:
            raw_line, _, remaining = self._serial_rx_buffer.partition(b'\n')
            self._serial_rx_buffer = bytearray(remaining)
            line = raw_line.decode(errors='ignore').strip()
            if line:
                self.get_logger().info(f'Arduino: {line}')

    def destroy_node(self):
        if hasattr(self, 'serial') and self.serial.is_open:
            self.send_drive_command(0, 0)
            self.write_command('PWM_OFF')
            self.serial.close()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = ArduinoBridge()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
