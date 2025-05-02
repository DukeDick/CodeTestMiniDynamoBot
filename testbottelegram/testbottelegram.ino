#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "driver/uart.h"
#include "string.h"

// ======= Wi-Fi and Bot Setup =======
const char* ssid = "__";
const char* password = "__";
const char* botToken = "__";

WiFiClientSecure secured_client;
UniversalTelegramBot bot(botToken, secured_client);

// ======= UART2 Configuration =======
#define UART_2_TX 17
#define UART_2_RX 16
static const int BUF_SIZE = 1024;

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

// ======= Telegram Timer =======
unsigned long last_check = 0;
const unsigned long interval = 2000;

void setup() {
  Serial.begin(115200);
  init_uart();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected");
  secured_client.setInsecure();  // Disable SSL certificate validation

  Serial.println("🤖 Telegram bot ready. Type a command (e.g., Walk, Run)");
}

void loop() {
  if (millis() - last_check > interval) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      for (int i = 0; i < numNewMessages; i++) {
        String command = bot.messages[i].text;

        // Clean command: remove all whitespace and line endings
        command.trim();
        command.replace("\n", "");
        command.replace("\r", "");

        // DO NOT append newline or null terminator

        // Send string as raw UART message
        uart_write_bytes(UART_NUM_2, command.c_str(), command.length());

        // Debug to Serial Monitor
        Serial.print("Sent to STM32: ");
        Serial.println(command);
        Serial.print("Byte stream: ");
        for (size_t j = 0; j < command.length(); j++) {
          Serial.print((int)command[j]);
          Serial.print(" ");
        }
        Serial.println();
      }

      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    last_check = millis();
  }
}










