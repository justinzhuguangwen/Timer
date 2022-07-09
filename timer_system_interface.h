// @brief 定时器Interface
//  @author justinzhu
//  @date 2022年6月28日18:16:35

#pragma once

#include <string>
#include "expiry_action.h"
#include "lib_time.h"

class TimerSystemInterface {
 public:
  virtual ~TimerSystemInterface() = default;

  virtual int Init(int64_t jiffies) = 0;
  virtual void RunTimers(int64_t jiffies) = 0;

  // interface:
  // @expires 超时时间，距离当前时间的Millis, 小于0的值会被修正为0
  // @interval 循环间隔Milliseconds, interval = 0表示非循环, 小于0的值会被修正为0
  // @return
  // 返回timer的globalid,
  // 可用dynamic_cast<Timer*>(CIDRuntimeClass::GetObjFromGlobalID(timer_globalid,
  // EOT_OBJ_TIMER))获取对象

  virtual int SetTimer(ExpiryAction* action, int64_t expires, int64_t interval = 0,
                       int64_t user_data = 0) = 0;

  // @expiry_time 超时Mills, 小于0的值会被修正为0
  // @examples:{Millis(100)}
  // {Sec(10), Millis(50)}
  // {Min(1), 0, Millis(50)}
  // {Hour(1), Min(1), Sec(10), Millis(50)}
  // {Millis(-100)}
  // {Sec(-1)}
  // {Min(10)}
  // @interval 循环间隔, interval = 0表示非循环, 小于0的值会被修正为0
  // @return
  // 返回timer的globalid,
  // 可用dynamic_cast<Timer*>(CIDRuntimeClass::GetObjFromGlobalID(timer_globalid,
  // EOT_OBJ_TIMER))获取对象
  virtual int SetTimer(ExpiryAction* action, TimeHelper expiry_time,
                       TimeHelper interval = {Millis(0)}, int64_t user_data = 0) {
    return SetTimer(action, expiry_time.GetMillis(), interval.GetMillis(), user_data);
  }

  // 清除timer
  // @timer_id timer的globalid
  virtual int ClearTimer(int32_t timer_id) = 0;

  // 重置timer
  // @timer_id timer的globalid
  // 其他参数同Start, 重置timer的参数, 以调用时刻重新计算超时
  // @return 0=success, <0=failed.
  virtual int ResetTimer(int32_t timer_id, ExpiryAction* action, int64_t expires,
                         int64_t interval = 0, int64_t user_data = 0) = 0;
  virtual int ResetTimer(int32_t timer_id, ExpiryAction* action, TimeHelper expiry_time,
                         TimeHelper interval = {Millis(0)}, int64_t user_data = 0) {
    return ResetTimer(timer_id, action, expiry_time.GetMillis(), interval.GetMillis(), user_data);
  }
};
