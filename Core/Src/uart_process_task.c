#include "uart_process_task.h"
#include "uart_buffer.h"
#include "dwt.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h" 
#include "main.h"
#include "usart.h"

// 定义日志TAG
#define LOG_TAG "uart_process"
#define LOG_LVL ELOG_LVL_DEBUG
#include "util.h"

// 任务参数
#define UART_PROCESS_STACK_SIZE 1024
#define UART_PROCESS_PRIORITY   osPriorityHigh

typedef StaticTask_t osStaticThreadDef_t;

// CMSIS-OS2任务句柄和相关资源
static osThreadId_t uart_process_task_handle = NULL;
static uint32_t uart_process_task_buffer[UART_PROCESS_STACK_SIZE/4];
static osStaticThreadDef_t uart_process_task_control_block;

// UART处理任务
static void uart_process_task(void *argument)
{
    uint8_t data_buffer[UART_RX_BUFFER_SIZE];
    uint32_t data_length;
    uint32_t rx_timestamp;
    uint32_t process_timestamp;
    
    log_i("UART process task started");
    uart_buffer_init(&huart1);
    uart_buffer_start_receive();
    
    for(;;)
    {
        // 等待接收数据信号量
        if (uart_buffer_wait_receive(UART_RECEIVE_TIMEOUT)) {
            // 读取数据和获取接收时间戳
            data_length = uart_buffer_read(data_buffer, UART_RX_BUFFER_SIZE, &rx_timestamp);
            
            if (data_length > 0) {
                // 记录处理时间戳
                process_timestamp = dwt_get_timestamp();
                
                log_d("Received %lu bytes, rx_ts=%lu, proc_ts=%lu", 
                      data_length, rx_timestamp, process_timestamp);
                
                // 发送包含时间戳和数据长度的响应
                uart_buffer_send_timestamp_response(rx_timestamp, process_timestamp, data_length);
            }
        }
    }
}

// 启动UART处理任务
void uart_process_task_start(void)
{
    // 定义任务属性
    osThreadAttr_t uart_process_attr = {
        .name = "UARTProcess",
        .cb_mem = &uart_process_task_control_block,
        .cb_size = sizeof(uart_process_task_control_block),
        .stack_mem = &uart_process_task_buffer[0],
        .stack_size = sizeof(uart_process_task_buffer),
        .priority = (osPriority_t)(UART_PROCESS_PRIORITY),
    };
    
    // 创建任务
    uart_process_task_handle = osThreadNew(uart_process_task, NULL, &uart_process_attr);
    log_i("uart task start!");
    
    if (uart_process_task_handle == NULL) {
        log_e("Failed to create UART process task");
    }
}