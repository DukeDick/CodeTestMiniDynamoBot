import rclpy
from rclpy.node import Node
from std_msgs.msg import String

class SimplePublisher(Node):
    def __init__(self):
        super().__init__('pc_command_publisher')
        self.publisher_ = self.create_publisher(String, 'PC_To_ESP32', 10)
        self.get_logger().info("✅ PC Publisher ready. Type a command below:")

        self.timer = self.create_timer(0.1, self.input_loop)

    def input_loop(self):
        try:
            user_input = input("Type command for STM32 via ESP32: ")
            command = user_input.strip()
            if command:
                msg = String()
                msg.data = command
                self.publisher_.publish(msg)
                self.get_logger().info(f"📤 Sent: '{command}'")
        except EOFError:
            pass

def main(args=None):
    rclpy.init(args=args)
    node = SimplePublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        print("👋 Shutting down...")
    finally:
        node.destroy_node()
        rclpy.shutdown()
