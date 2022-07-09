// @brief 定时器的一些基本定义，参考自Linux Timer:
//  @author justinzhu
//  @date 2022年6月28日18:16:35
//
#pragma once

#include <stdint.h>

// timer vector definitions:

#define TVN_BITS (6)
#define TVR_BITS (8)
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)
#define MAX_TVAL ((int64_t)((1ULL << (TVR_BITS + 4 * TVN_BITS)) - 1))
