#include "dwt.h"

// 定义日志TAG
#define LOG_TAG "dwt"
#define LOG_LVL ELOG_LVL_DEBUG
#include "util.h"

void dwt_init(void)
{
    // 启用DWT外设
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    // 清零计数器
    DWT->CYCCNT = 0;
    
    // 启用计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    log_i("DWT timer initialized");
}

uint32_t dwt_get_timestamp(void)
{
    // 返回当前DWT计数器值
    return DWT->CYCCNT;
}
