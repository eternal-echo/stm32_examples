#ifndef UART_BUFFER_H
#define UART_BUFFER_H

#include "main.h"

// 缓冲区大小定义
#define UART_RX_BUFFER_SIZE  256
#define UART_BUFFER_COUNT    2   // 双缓冲

// 等待接收数据的超时时间(ms)
#define UART_RECEIVE_TIMEOUT 1000

// 缓冲区数据结构
typedef struct {
    uint8_t data[UART_RX_BUFFER_SIZE];
    uint32_t length;
    uint32_t timestamp;
} uart_buffer_t;

// 初始化串口缓冲区和信号量
void uart_buffer_init(UART_HandleTypeDef *huart);

// 启动串口接收
void uart_buffer_start_receive(void);

// 获取一个已接收的缓冲区
uart_buffer_t* uart_buffer_get_rxdata(uint32_t timeout);

// 发送数据
uint32_t uart_buffer_send(uint8_t *data, uint32_t length);

// 发送时间戳响应
void uart_buffer_send_timestamp_response(uint32_t rx_timestamp, uint32_t process_timestamp, uint32_t data_length);

#endif // UART_BUFFER_H