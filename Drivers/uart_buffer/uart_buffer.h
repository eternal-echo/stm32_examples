#ifndef UART_BUFFER_H
#define UART_BUFFER_H

#include "main.h"

// 缓冲区大小定义
#define UART_RX_BUFFER_SIZE  256

// 初始化串口缓冲区
void uart_buffer_init(UART_HandleTypeDef *huart);

// 启动串口接收(开启中断接收)
void uart_buffer_start_receive(void);

// 获取接收缓冲区中可读数据长度
uint32_t uart_buffer_available(void);

// 读取缓冲区中的数据，同时获取接收时间戳
uint32_t uart_buffer_read(uint8_t *data, uint32_t length, uint32_t *rx_timestamp);

// 发送数据
uint32_t uart_buffer_send(uint8_t *data, uint32_t length);

// 发送时间戳和数据长度信息的响应包
void uart_buffer_send_timestamp_response(uint32_t rx_timestamp, uint32_t process_timestamp, uint32_t data_length);

// // UART接收中断回调函数 - 在HAL_UART_RxCpltCallback中调用
// void uart_buffer_rx_callback(UART_HandleTypeDef *huart);

#endif // UART_BUFFER_H