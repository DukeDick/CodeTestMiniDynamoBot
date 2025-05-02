#include <WiFi.h>
#include <HTTPClient.h>
#include "driver/uart.h"

const char* ssid = "__";
const char* password = "__3";

const char* server_url = "http://192.___._.__:8000/get_command";  // Flask server on PC

#define UART_2_TX 17
#define UART_2_RX 16
#define BUF_SIZE 1024

void init_uart() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };
  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2, UART_2_TX, UART_2_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_2, BUF_SIZE, 0, 0, NULL, 0);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");

  init_uart();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server_url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      int cmdStart = payload.indexOf(":\"") + 2;
      int cmdEnd = payload.indexOf("\"", cmdStart);
      String command = payload.substring(cmdStart, cmdEnd);

      if (command.length() > 0) {
        Serial.println("📥 Command received: " + command);
        uart_write_bytes(UART_NUM_2, command.c_str(), command.length());
      }
    } else {
      Serial.println("Server response error: " + String(httpCode));
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  delay(3000);  // Poll every 3 seconds
}
