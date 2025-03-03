#include "elog.h"
#include "cmsis_os2.h"

/* 定义任务句柄和信号量 */
static osThreadId_t elog_async_thread = NULL;
static osSemaphoreId_t elog_async_notice = NULL;

/* 异步输出任务的栈大小 */
#define ELOG_ASYNC_TASK_STACK_SIZE 512

/* 声明端口输出函数 */
extern void elog_port_output(const char *log, size_t size);

/* 异步输出任务 */
static void elog_async_task(void *arg) {
    size_t get_log_size = 0;
    static char poll_get_buf[ELOG_ASYNC_OUTPUT_BUF_SIZE - 4];

    (void)arg; /* 避免未使用参数警告 */

    while(1) {
        /* 等待日志通知 */
        osSemaphoreAcquire(elog_async_notice, osWaitForever);
        
        /* 轮询获取并输出日志 */
        while(1) {
#ifdef ELOG_ASYNC_LINE_OUTPUT
            get_log_size = elog_async_get_line_log(poll_get_buf, sizeof(poll_get_buf));
#else
            get_log_size = elog_async_get_log(poll_get_buf, sizeof(poll_get_buf));
#endif
            if (get_log_size) {
                elog_port_output(poll_get_buf, get_log_size);
            } else {
                break;
            }
        }
    }
}

/* 异步输出通知函数，在有新日志时被调用 */
void elog_async_output_notice(void) {
    osSemaphoreRelease(elog_async_notice);
}

/* 初始化异步输出任务和信号量 */
ElogErrCode elog_async_port_init(void) {
    /* 创建二值信号量 */
    elog_async_notice = osSemaphoreNew(1, 0, NULL);
    if (elog_async_notice == NULL) {
        return ELOG_ERR;
    }
    
    /* 创建异步输出任务 */
    osThreadAttr_t thread_attr = {
        .name = "elog_async",
        .stack_size = ELOG_ASYNC_TASK_STACK_SIZE,
        .priority = osPriorityNormal,
    };
    elog_async_thread = osThreadNew(elog_async_task, NULL, &thread_attr);
    if (elog_async_thread == NULL) {
        return ELOG_ERR;
    }
    
    return ELOG_NO_ERR;
}