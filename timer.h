// @brief 定时器, 参考自Linux(4.0) Timer,
// https://github.com/torvalds/linux/blob/master/kernel/time/timer.c
//  @author justinzhu
//  @date 2022年6月28日18:16:35

#pragma once

#include <functional>
#include "comm/misc/type_table_base.h"
#include "comm_base.h"
#include "comm_obj_list_head.h"
#include "comm_object.h"
#include "expiry_action.h"
#include "lib_str.h"
#include "timer_defines.h"

// Timer定义
// @CObj 共享内存存储，可恢复
// @ListHead<Timer> Timer同时是个链表节点
class Timer : public CObj, public ListHead<Timer> {
 public:
  Timer();
  virtual ~Timer() = default;
  std::string DebugString() {
    // https://stackoverflow.com/questions/18039723/c-trying-to-get-function-address-from-a-stdfunction
    return format_string(
        "(globalid:%d, self:%d, prev:%d, next:%d action:%p, expires:%ld, interval:%ld, "
        "user_data:%ld)",
        GetGlobalID(), Self(), Prev(), Next(), reinterpret_cast<void *>(action_), expires_,
        interval_, user_data_);
  }

  int64_t Expires() { return expires_; }
  int64_t Interval() { return interval_; }
  ExpiryAction *Action() { return action_; }
  int64_t UserData() { return user_data_; }

 protected:
  friend class TimerSystem;
  // 初始化Timer函数
  // @param expires 超时时间点
  // @param interval 循环型间隔时间
  // @param user_data 用户数据
  void Init(ExpiryAction *action, int64_t expires, int64_t interval = 0, int64_t user_data = 0) {
    action_ = action;
    expires_ = expires;
    interval_ = interval;
    user_data_ = user_data;
    SetNext(LIST_POISON);
  }

  void SetExpires(int64_t expires) { expires_ = expires; }
  void SetInterval(int64_t interval) { interval_ = interval; }
  void SetAction(ExpiryAction *action) { action_ = action; }
  void SetUserData(int64_t user_data) { user_data_ = user_data; }

  void CreateInit();
  void ResumeInit();

 protected:
  // 将Timer从链表里移除
  void DetachTimer(bool clear_pending) {
    InternalListDel(GetPrevObject(), GetNextObject());
    if (clear_pending)
      SetNext(LIST_POISON);
    SetPrev(LIST_POISON);
  }

  // 判断timer是不是已经在列表里
  bool TimerPending() { return Next() >= 0; }

 private:
  int64_t expires_;       // 超时时间点
  int64_t interval_;      // 循环型的间隔时间
  ExpiryAction *action_;  // 调用者的this指针
  int64_t user_data_;     // 用户数据

  DECLARE_IDCREATE(Timer);
};
