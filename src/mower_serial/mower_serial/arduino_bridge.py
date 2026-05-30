import time

import rclpy
from rclpy.node import Node
import serial
from std_msgs.msg import String


class ArduinoBridge(Node):
    def __init__(self):
        super().__init__('arduino_bridge')

        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baud', 115200)

        port = self.get_parameter('port').value
        baud = self.get_parameter('baud').value

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
        self.serial_timer = self.create_timer(0.05, self.read_serial)
        self.heartbeat_timer = self.create_timer(0.5, self.send_heartbeat)
        self.send_heartbeat()

        self.get_logger().info('Debug bridge ready. Publish String commands to /mower/arduino_cmd')

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
