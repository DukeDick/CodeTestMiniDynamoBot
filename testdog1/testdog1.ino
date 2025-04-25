#include "driver/uart.h"
#include "string.h"

#define UART_2_TX 17
#define UART_2_RX 16
static const int BUF_SIZE = 1024;

void init_uart()
{
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

void setup() {
    Serial.begin(115200);
    init_uart();
}

void loop() {
    const char *tx_data = "Walk";
    uart_write_bytes(UART_NUM_2, tx_data, strlen(tx_data));
    Serial.println("Sent: Walk");
    delay(5000);  // wait 5 seconds
}


