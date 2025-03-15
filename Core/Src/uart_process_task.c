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
    uart_buffer_t *rx_buffer;
    uint32_t process_timestamp;
    
    log_i("UART process task started");
    uart_buffer_init(&huart1);
    uart_buffer_start_receive();
    
    for(;;)
    {
        // 等待接收缓冲区数据
        rx_buffer = uart_buffer_get_rxdata(UART_RECEIVE_TIMEOUT);
        if (rx_buffer != NULL && rx_buffer->length > 0) {
            // 记录处理时间戳
            process_timestamp = dwt_get_timestamp();
            
            log_d("Received %lu bytes, rx_ts=%lu, proc_ts=%lu", 
                  rx_buffer->length, rx_buffer->timestamp, process_timestamp);
            
            // 发送响应
            uart_buffer_send_timestamp_response(
                rx_buffer->timestamp, 
                process_timestamp, 
                rx_buffer->length);
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