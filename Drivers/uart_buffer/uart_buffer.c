#include "uart_buffer.h"
#include <string.h>

// 定义一个日志TAG
#define LOG_TAG "uart_buffer"
#define LOG_LVL ELOG_LVL_DEBUG
#include "util.h"

// 串口接收缓冲区
static UART_HandleTypeDef *g_huart;
static uint8_t g_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint32_t g_rx_index = 0;
static uint8_t g_rx_temp; // 用于单字节接收的临时变量

// 初始化串口缓冲区
void uart_buffer_init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
    g_rx_index = 0;
    memset(g_rx_buffer, 0, UART_RX_BUFFER_SIZE);
    
    log_i("UART buffer initialized");
}

// 启动串口接收
void uart_buffer_start_receive(void)
{
    if (g_huart != NULL) {
        // 启动中断接收第一个字节
        HAL_UART_Receive_IT(g_huart, &g_rx_temp, 1);
        log_i("UART receive started");
    }
}

// 获取接收缓冲区中可读数据长度
uint32_t uart_buffer_available(void)
{
    return g_rx_index;
}

// 读取缓冲区中的数据
uint32_t uart_buffer_read(uint8_t *data, uint32_t length)
{
    uint32_t read_len = 0;
    
    if (data == NULL || length == 0 || g_rx_index == 0)
        return 0;
    
    // 计算实际可读取的长度
    read_len = (length > g_rx_index) ? g_rx_index : length;
    
    // 复制数据
    memcpy(data, g_rx_buffer, read_len);
    
    // 如果未读完所有数据，则移动剩余数据到缓冲区起始位置
    if (read_len < g_rx_index) {
        memmove(g_rx_buffer, g_rx_buffer + read_len, g_rx_index - read_len);
    }
    
    // 更新索引
    g_rx_index -= read_len;
    
    return read_len;
}

// 发送数据 (阻塞方式)
uint32_t uart_buffer_send(uint8_t *data, uint32_t length)
{
    if (g_huart == NULL || data == NULL || length == 0)
        return 0;
    
    HAL_StatusTypeDef status = HAL_UART_Transmit(g_huart, data, length, 100);
    
    if (status == HAL_OK)
        return length;
    else
        return 0;
}

// UART接收中断回调函数
void uart_buffer_rx_callback(UART_HandleTypeDef *huart)
{
    if (huart != g_huart)
        return;
    
    // 检查接收缓冲区是否已满
    if (g_rx_index < UART_RX_BUFFER_SIZE) {
        // 存储接收到的字节
        g_rx_buffer[g_rx_index++] = g_rx_temp;
    } else {
        // 缓冲区已满，可以选择丢弃数据或进行其他处理
        log_w("UART RX buffer overflow");
    }
    
    // 继续接收下一个字节
    HAL_UART_Receive_IT(g_huart, &g_rx_temp, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* USER CODE BEGIN HAL_UART_RxCpltCallback */
  uart_buffer_rx_callback(huart);
  /* USER CODE END HAL_UART_RxCpltCallback */
}