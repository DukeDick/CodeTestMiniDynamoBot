#include <WiFi.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/string.h>
#include <micro_ros_utilities/type_utilities.h>
#include <micro_ros_utilities/string_utilities.h>
#include "driver/uart.h"
#include "string.h"

#define UART_2_TX 17
#define UART_2_RX 16
#define BUF_SIZE 1024
#define LED_PIN 2

// Wi-Fi credentials
const char* ssid = "DREAMSLAB UNIFI";
const char* password = "@DREAMSLAB2023";

// micro-ROS agent IP and port
const char* agent_ip = "192.168.0.23";
const uint16_t agent_port = 8888;

// micro-ROS entities
rcl_node_t node;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_subscription_t subscriber;
rclc_executor_t executor;
std_msgs__msg__String incoming_msg;
static micro_ros_utilities_memory_conf_t conf = {0};

void init_uart() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB
  };
  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2, UART_2_TX, UART_2_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void blink() {
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
}

void message_callback(const void* msgin) {
  const std_msgs__msg__String* msg = (const std_msgs__msg__String*)msgin;
  const char* cmd = micro_ros_string_utilities_get_c_str(msg->data);
  uart_write_bytes(UART_NUM_2, cmd, strlen(cmd));
  blink();
  Serial.print("📩 Received and sent to STM32 via UART2: ");
  Serial.println(cmd);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  init_uart();
  delay(2000);

  Serial.println("🌐 Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi connected");

  set_microros_wifi_transports((char*)ssid, (char*)password, (char*)agent_ip, agent_port);

  conf.max_string_capacity = 64;

  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "esp32_led_subscriber", "", &support);

  bool success = micro_ros_utilities_create_message_memory(
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
    &incoming_msg,
    conf
  );

  if (!success) {
    Serial.println("❌ Failed to allocate message memory");
    while (1);
  }

  rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
    "led_command"
  );

  rclc_executor_init(&executor, &support.context, 1, &allocator);
  rclc_executor_add_subscription(&executor, &subscriber, &incoming_msg, &message_callback, ON_NEW_DATA);
}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  delay(10);
}
