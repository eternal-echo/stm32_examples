#include "uart_buffer.h"
#include "dwt.h"
#include "cmsis_os2.h"
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
static uint32_t g_rx_timestamp = 0; // 记录接收时间戳

// 定义二值信号量句柄
static osSemaphoreId_t rx_sem = NULL;

// 初始化串口缓冲区
void uart_buffer_init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
    g_rx_index = 0;
    memset(g_rx_buffer, 0, UART_RX_BUFFER_SIZE);
    
    // 创建二值信号量
    rx_sem = osSemaphoreNew(1, 0, NULL);
    if (rx_sem == NULL) {
        log_e("Failed to create rx semaphore");
        return;
    }
    
    // 初始化DWT计时器
    dwt_init();
    
    log_i("UART buffer initialized with semaphore");
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

// 读取缓冲区中的数据并获取接收时间戳
uint32_t uart_buffer_read(uint8_t *data, uint32_t length, uint32_t *rx_timestamp)
{
    uint32_t read_len = 0;
    
    if (data == NULL || length == 0 || g_rx_index == 0)
        return 0;
    
    // 计算实际可读取的长度
    read_len = (length > g_rx_index) ? g_rx_index : length;
    
    // 复制数据
    memcpy(data, g_rx_buffer, read_len);
    
    // 如果提供了时间戳指针，则保存接收时间戳
    if (rx_timestamp != NULL) {
        *rx_timestamp = g_rx_timestamp;
    }
    
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

// 发送时间戳和数据长度信息
void uart_buffer_send_timestamp_response(uint32_t rx_timestamp, uint32_t process_timestamp, uint32_t data_length)
{
    // 创建响应数据包
    typedef struct {
        uint32_t rx_ts;      // 接收时间戳
        uint32_t proc_ts;    // 处理时间戳
        uint32_t delta_us;   // 时间差（微秒）
        uint32_t data_len;   // 数据长度
    } timestamp_response_t;
    
    timestamp_response_t response;
    response.rx_ts = rx_timestamp;
    response.proc_ts = process_timestamp;
    
    // 计算时间差并转换为微秒 (168MHz = 168 ticks per us)
    uint32_t delta_ticks = process_timestamp - rx_timestamp;
    response.delta_us = delta_ticks / 168;
    response.data_len = data_length;
    
    // 发送响应
    uart_buffer_send((uint8_t*)&response, sizeof(timestamp_response_t));
    log_d("Sent timestamp response: rx=%u, proc=%u, delta=%u us, len=%u", 
           rx_timestamp, process_timestamp, response.delta_us, data_length);
}

// UART接收中断回调函数
void uart_buffer_rx_callback(UART_HandleTypeDef *huart)
{
    if (huart != g_huart)
        return;
    
    // 当第一个字节被接收时记录时间戳
    if (g_rx_index == 0) {
        g_rx_timestamp = dwt_get_timestamp();
    }
    
    if (g_rx_index < UART_RX_BUFFER_SIZE) {
        g_rx_buffer[g_rx_index++] = g_rx_temp;
        
        // 释放信号量通知任务
        osSemaphoreRelease(rx_sem);
    } else {
        log_w("UART RX buffer overflow");
    }
    
    // 继续接收下一个字节
    HAL_UART_Receive_IT(g_huart, &g_rx_temp, 1);
}

uint8_t uart_buffer_wait_receive(uint32_t timeout)
{
    // 等待信号量
    return (osSemaphoreAcquire(rx_sem, timeout) == osOK) ? 1 : 0;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* USER CODE BEGIN HAL_UART_RxCpltCallback */
  uart_buffer_rx_callback(huart);
  /* USER CODE END HAL_UART_RxCpltCallback */
}