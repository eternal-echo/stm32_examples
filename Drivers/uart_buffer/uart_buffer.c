#include "uart_buffer.h"
#include "dwt.h"
#include "cmsis_os2.h"
#include <string.h>

// 定义日志TAG
#define LOG_TAG "uart_buffer"
#define LOG_LVL ELOG_LVL_DEBUG
#include "util.h"

// 串口相关变量
static UART_HandleTypeDef *g_huart;
static uart_buffer_t g_rx_buffers[UART_BUFFER_COUNT];
static volatile uint8_t g_current_buffer = 0;  // 当前接收缓冲区
static volatile uint8_t g_process_buffer = 0;  // 当前处理缓冲区

// 信号量句柄
static osSemaphoreId_t rx_sem = NULL;

void uart_buffer_init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
    memset(g_rx_buffers, 0, sizeof(g_rx_buffers));
    
    // 创建二值信号量
    rx_sem = osSemaphoreNew(1, 0, NULL);
    if (rx_sem == NULL) {
        log_e("Failed to create rx semaphore");
        return;
    }
    
    // 初始化DWT计时器
    dwt_init();
    
    log_i("UART buffer initialized with double buffering");
}

void uart_buffer_start_receive(void)
{
    if (g_huart != NULL) {
        // 开始接收，使用HAL_UARTEx_ReceiveToIdle_IT
        if (HAL_UARTEx_ReceiveToIdle_IT(g_huart, 
            g_rx_buffers[g_current_buffer].data, 
            UART_RX_BUFFER_SIZE) != HAL_OK) {
            log_e("Failed to start UART receive");
        }
        log_i("UART receive started with buffer %d", g_current_buffer);
    }
}

uart_buffer_t* uart_buffer_get_rxdata(uint32_t timeout)
{
    if (osSemaphoreAcquire(rx_sem, timeout) == osOK) {
        return &g_rx_buffers[g_process_buffer];
    }
    return NULL;
}

// 获取接收缓冲区中可读数据长度
uint32_t uart_buffer_available(void)
{
    return g_rx_buffers[g_process_buffer].length;
}

// 读取缓冲区中的数据并获取接收时间戳
uint32_t uart_buffer_read(uint8_t *data, uint32_t length, uint32_t *rx_timestamp)
{
    uint32_t read_len = 0;
    
    if (data == NULL || length == 0 || g_rx_buffers[g_process_buffer].length == 0)
        return 0;
    
    // 计算实际可读取的长度
    read_len = (length > g_rx_buffers[g_process_buffer].length) ? g_rx_buffers[g_process_buffer].length : length;
    
    // 复制数据
    memcpy(data, g_rx_buffers[g_process_buffer].data, read_len);
    
    // 如果提供了时间戳指针，则保存接收时间戳
    if (rx_timestamp != NULL) {
        *rx_timestamp = g_rx_buffers[g_process_buffer].timestamp;
    }
    
    // 更新索引
    g_rx_buffers[g_process_buffer].length -= read_len;
    
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
void uart_buffer_rx_callback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart != g_huart)
        return;
        
    // 记录接收时间戳和长度
    g_rx_buffers[g_current_buffer].timestamp = dwt_get_timestamp();
    g_rx_buffers[g_current_buffer].length = Size;
    
    // 切换处理缓冲区
    g_process_buffer = g_current_buffer;
    
    // 切换到另一个缓冲区继续接收
    g_current_buffer = (g_current_buffer + 1) % UART_BUFFER_COUNT;
    
    // 释放信号量通知处理任务
    osSemaphoreRelease(rx_sem);
    
    // 启动下一次接收
    HAL_UARTEx_ReceiveToIdle_IT(huart, 
        g_rx_buffers[g_current_buffer].data, 
        UART_RX_BUFFER_SIZE);
}

uint8_t uart_buffer_wait_receive(uint32_t timeout)
{
    // 等待信号量
    return (osSemaphoreAcquire(rx_sem, timeout) == osOK) ? 1 : 0;
}
/**
  * @brief  Reception Event Callback (Rx event notification called after use of advanced reception service).
  * @param  huart UART handle
  * @param  Size  Number of data available in application reception buffer (indicates a position in
  *               reception buffer until which, data are available)
  * @retval None
  */
 void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
 {
   /* Prevent unused argument(s) compilation warning */
 
   /* NOTE : This function should not be modified, when the callback is needed,
             the HAL_UARTEx_RxEventCallback can be implemented in the user file.
    */
   uart_buffer_rx_callback(huart, Size);
 }