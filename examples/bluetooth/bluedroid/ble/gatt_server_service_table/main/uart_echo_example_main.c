/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "gatts_table_creat_demo.h"
#include "uart_echo_example_main.h"



esp_err_t  write_transmit_method(TransmitMethod method);


/**
 * This is an example which echos any data it receives on UART1 back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: UART1
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below
 */

#define ECHO_UART_NUM  (UART_NUM_1)
#define ECHO_TEST_TXD  (GPIO_NUM_23)
#define ECHO_TEST_RXD  (GPIO_NUM_22)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

#define  AT_ONLY              "at"
#define  AT_TRANSMIT_METHOD_STR  "method"
#define  AT_TRANSMIT_METHOD   "at+"AT_TRANSMIT_METHOD_STR"="
#define  AT_SEND_DATA_CMD_STR "sdata"
#define  AT_SEND_DATA         "at+"AT_SEND_DATA_CMD_STR"="

static const char *at_ok  = "ok\r\n";
static const char *at_err = "error\r\n";
static uint8_t tmp_buf[BUF_SIZE];

static void hexdump(const char *hdr, const uint8_t *data, int len) {
    printf("%s ", hdr);
    for(int i=0; i<len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

int pop_one_command(char *data, char *crlf, int *p_offset) {
    char *next_cmd = crlf;
    for(int i=0; i<2 && next_cmd < (data + *p_offset); i++) {
        if(*next_cmd == '\r' || *next_cmd == '\n') {
            next_cmd++;
        } else {
            break;
        }
    }

    int remain_len = data + *p_offset - next_cmd;
    memcpy(tmp_buf, next_cmd, remain_len);
    memcpy(data, tmp_buf, remain_len);
    *p_offset = remain_len;
    return remain_len;
}

int at_handler(char *data, int *p_offset) {
    uart_write_bytes(ECHO_UART_NUM, at_ok, strlen(at_ok));
    return 0;
}

int at_transmit_method_handler(char *data, int *p_offset) {
    int value = atoi(data + strlen(AT_TRANSMIT_METHOD));
    if(write_transmit_method((TransmitMethod)value) == ESP_OK) {
        uart_write_bytes(ECHO_UART_NUM, at_ok, strlen(at_ok));
        vTaskDelay(1000 / portTICK_RATE_MS);
        esp_restart();
    } else {
        uart_write_bytes(ECHO_UART_NUM, at_err, strlen(at_err));
    }
    return 0;
}


int at_send_data_handler(char *data, int *p_offset, int data_len, char *crlf) {
    uint8_t *data_buf = (uint8_t *)strchr(data + strlen(AT_SEND_DATA), ',') + 1;
    if( ble_notify_interface_send(data_buf, data_len) != ESP_OK ) {
        uart_write_bytes(ECHO_UART_NUM, at_err, strlen(at_err));
    } else {
        uart_write_bytes(ECHO_UART_NUM, at_ok, strlen(at_ok));
    }
    return pop_one_command(data, crlf, p_offset);
}

void remove_space_prefix(char *data, int *p_offset) {
    char *not_space;
    for(not_space = data; not_space<(data + p_offset[0]); not_space++) {
        if(!isspace(not_space[0])) {
            break;
        }
    }
    if(not_space != data) {
        int remain_len = data + *p_offset - not_space;
        memcpy(tmp_buf, not_space, remain_len);
        memcpy(data, tmp_buf, remain_len);
        *p_offset = remain_len;
    }
}

/**
 * 0:   没有数据需要再处理
 * 非0：还有数据
 */
int handle_uart_data(char *data, int *p_offset) {    
    remove_space_prefix(data, p_offset);
    if(0 == p_offset[0]) {
        /* no data */
        return 0;
    }

    //hexdump("now", (uint8_t *)data, *p_offset);
    /* at+sdata= */
    if(!strncasecmp(data, AT_SEND_DATA, strlen(AT_SEND_DATA))) {
        /* 发送数据 */
        int data_len = atoi(data + strlen(AT_SEND_DATA));
        //printf("data_len = %d\n", data_len);
        if(data + p_offset[0] <= (strchr(data + strlen(AT_SEND_DATA), ',') + 1 + data_len) ) {
            /* 命令未完整 */
            //printf("sdata cmd not completed\n");
            return 0;
        }

        /* 命令完整了 */
        char *after_data = strchr(data + strlen(AT_SEND_DATA), ',') + 1 + data_len;
        if( after_data[0] != '\r' && after_data[0] != '\n' ) {
            uart_write_bytes(ECHO_UART_NUM, at_err, strlen(at_err));
            /* 格式错误，重置接受缓存 */
            p_offset[0] = 0;
            return 0;
        }
        /* correct */
        return at_send_data_handler(data, p_offset, data_len, after_data);
    }

    /* 常规命令 */
    char *crlf = memmem(data, p_offset[0], "\r", 1);
    if(NULL == crlf) {
        crlf = memmem(data, p_offset[0], "\n", 1);
    }

    if(NULL == crlf) {
        /* 命令不完整 */
#if  0
        uart_write_bytes(ECHO_UART_NUM, at_err, strlen(at_err));
        /* 格式错误，重置接受缓存 */
        p_offset[0] = 0;
#endif
        return 0;
    } else if(!strncasecmp(data, AT_ONLY, strlen(AT_ONLY)) && (data[strlen(AT_ONLY)] == '\r' || data[strlen(AT_ONLY)] == '\n' )) {
        at_handler(data, p_offset);
    } else if(!strncasecmp(data, AT_TRANSMIT_METHOD, strlen(AT_TRANSMIT_METHOD))) {
        at_transmit_method_handler(data, p_offset);
    } else {
        /* invalid AT command */
        uart_write_bytes(ECHO_UART_NUM, at_err, strlen(at_err));
    }
    return pop_one_command(data, crlf, p_offset);
}

static void echo_task()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(ECHO_UART_NUM, &uart_config);
    uart_set_pin(ECHO_UART_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(ECHO_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Configure a temporary buffer for the incoming data
    char *data = (char *) malloc(BUF_SIZE);
    int offset = 0;

    for (;;) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_NUM, (uint8_t *)data + offset, BUF_SIZE - offset, 20 / portTICK_RATE_MS);
        if(len <= 0) {
            continue;
        }

        /* 收到数据 */
        offset += len;
        //hexdump("recv:", (const uint8_t *)data + offset - len, len);
        //printf("offset = %d\n", offset);
        if(memmem(data + offset - len, len, "\r", 1) || memmem(data + offset - len, len, "\n", 1)) {
            while(handle_uart_data(data, &offset));
        }
    }
}

void uart_app_main()
{
    //xTaskCreate(echo_task, "uart_echo_task", 1024, NULL, 10, NULL);
    echo_task();
}

