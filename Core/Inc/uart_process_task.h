#ifndef UART_PROCESS_TASK_H
#define UART_PROCESS_TASK_H

#include "main.h"

/**
 * @brief 启动UART处理任务
 * 
 * 创建并启动用于处理UART缓冲区数据的任务
 */
void uart_process_task_start(void);

#endif /* UART_PROCESS_TASK_H */
