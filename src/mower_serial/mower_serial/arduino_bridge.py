import time
import serial 

import rclpy
from rclpy.node import Node 
from std_msgs.msg import String

class ArduinoBridge(Node):
    def __init__(self):
        super().__init__('arduino_bridge')

        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baud', 115200)

        port = self.get_parameter('port').value
        baud = self.get_parameter('baud').value

        self.get_logger().info(f'Opening serial port {port} at {baud} baud')
        self.serial = serial.Serial(port, baud, timeout = 1)
        time.sleep(2)

        self.subscription = self.create_subscription(
            String,
            '/mower/arduino_cmd',
            self.command_callback,
            10
        )

        self.get_logger().info('Ready. Publish String commands to /mower/arduino_cmd')

    def command_callback(self, msg):
        command = msg.data.strip()
        if not command:
            return

        self.serial.write((command + '\n').encode('utf-8'))

        response = self.serial.readline().decode(errors='ignore').strip()
        if response:
            self.get_logger().info(f'Arduino replied: {response}')
        else:
            self.get_logger().warn('No response from Arduino')

    def destroy_node(self):
        if hasattr(self, 'serial') and self.serial.is_open:
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
