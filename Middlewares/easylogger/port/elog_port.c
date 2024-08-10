/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */

#include <elog.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "main.h"
#include "usart.h"

typedef StaticSemaphore_t osStaticMutexDef_t;

osMutexId_t elog_lockHandle;
osStaticMutexDef_t elog_lockControlBlock;
const osMutexAttr_t elog_lock_attributes = {
  .name = "elog_lock",
  .attr_bits = osMutexRecursive,
  .cb_mem = &elog_lockControlBlock,
  .cb_size = sizeof(elog_lockControlBlock),
};


int serial_send(const uint8_t *data, uint16_t size) {
    HAL_StatusTypeDef status = HAL_OK;
    while (HAL_UART_GetState(&log_huart) != HAL_UART_STATE_READY) {
        osDelay(1);
    }
    status = HAL_UART_Transmit(&log_huart, data, size, 1000);
    if (status != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
    elog_lockHandle = osMutexNew(&elog_lock_attributes);
    if (elog_lockHandle == NULL) {
        result = ELOG_ERR;
    }

    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {
    osMutexDelete(elog_lockHandle);
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    serial_send((const uint8_t *) log, size);
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    osMutexAcquire(elog_lockHandle, osWaitForever);
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    osMutexRelease(elog_lockHandle);
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
    static char cur_system_time[16] = "";
    snprintf(cur_system_time, 16, "%lu", osKernelGetTickCount());
    return cur_system_time;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    return "";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    return osThreadGetName(osThreadGetId());
}