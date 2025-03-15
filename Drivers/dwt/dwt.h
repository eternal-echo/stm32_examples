#ifndef DWT_H
#define DWT_H

#include "main.h"

// 初始化DWT计时器
void dwt_init(void);

// 获取DWT计时器当前值（时间戳）
uint32_t dwt_get_timestamp(void);

#endif // DWT_H
