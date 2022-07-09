// @brief 定时器, 参考自Linux Timer:linux/include/linux/timer.h?c
//  @author justinzhu
//  @date 2022年6月28日18:16:35
///

#pragma once

#include "comm_base.h"
#include "comm_service_interface.h"
#include "timer.h"
#include "timer_defines.h"
#include "timer_system_interface.h"

struct tvec {
  int32_t vec[TVN_SIZE];  // store list head obj
};

struct tvec_root {
  int32_t vec[TVR_SIZE];  // store list head obj
};

class TimerSystem : public CObj, public TimerSystemInterface, public IService {
 public:
  TimerSystem();
  virtual ~TimerSystem();
  virtual const char* ClassName() { return "TimerSystem"; }
  void CreateInit();
  void ResumeInit() {}

 public:
  // @expires 超时时间，距离当前时间的Millis, 小于0的值会被修正为0
  // @interval 循环间隔Milliseconds, interval = 0表示非循环, 小于0的值会被修正为0
  // @return 返回timer的globalid,
  // 用dynamic_cast<Timer*>(CIDRuntimeClass::GetObjFromGlobalID(timer_globalid,
  // EOT_OBJ_TIMER))获取对象
  virtual int SetTimer(ExpiryAction* action, int64_t expires, int64_t interval = 0,
                       int64_t user_data = 0) override;

  // @timer_id timer的globalid
  virtual int ClearTimer(int32_t timer_id) override;

  // @timer_id timer的globalid
  // 其他参数同SetTimer, 重置timer的参数, 以调用时刻重新计算超时
  // @return 0=success, <0=failed.
  virtual int ResetTimer(int32_t timer_id, ExpiryAction* action, int64_t expires,
                         int64_t interval = 0, int64_t user_data = 0) override;

 public:
  int Init(int64_t jiffies);
  void RunTimers(int64_t jiffies);

 public:
  void AddTimer(Timer* timer, int64_t jiffies);
  int DelTimer(Timer* timer, int64_t jiffies);

  int ModTimer(Timer* timer, int64_t jiffies, int64_t expires);
  int ModTimerPending(Timer* timer, int64_t jiffies, int64_t expires);

 public:
  int64_t AllTimers() { return all_timers_; }

 private:
  void InternalAddTimer(Timer* timer, int64_t jiffies);
  void DoInternalAddTimer(Timer* timer);
  int DetachIfPending(Timer* timer, bool clear_pending, int64_t jiffies);
  int InternalModTimer(Timer* timer, int64_t jiffies, int64_t expires, bool pending_only);

  void DetachExpiredTimer(Timer* timer, int64_t jiffies);
  bool CatchupTimerJiffies(int64_t jiffies);
  int Cascade(struct tvec* tv, int index);

 private:
  int64_t timer_jiffies_;  // 当前jiffies
  int64_t next_timer_;     // 最近超时timer的jiffies
  int64_t active_timers_;  // 活跃timer计数
  int64_t all_timers_;     // timers 总计数
  // 这里tv1~tv5分别是时间轮的5级轮盘Linux定时器时间轮分为5个级别的轮子(tv1 ~ tv5)。
  // 每个级别的轮子的刻度值(slot)不同，规律是次级轮子的slot等于上级轮子的slot之和。
  // Linux定时器slot单位为1jiffy，tv1轮子分256个刻度，每个刻度大小为1jiffy。
  // tv2轮子分64个刻度，每个刻度大小为256个jiffy，即tv1整个轮子所能表达的范围。
  // 相邻轮子也只有满足这个规律，才能达到“低刻度轮子转一圈，高刻度轮子走一格”的效果。
  // tv3，tv4，tv5也都是分为64个刻度，因此容易算出，最高一级轮子tv5所能表达的slot范围达到了
  // 256*64*64*64*64 = 2^32 jiffies。
  // https://iwiki.woa.com/pages/viewpage.action?pageId=1889841580///
  struct tvec_root tv1_;
  struct tvec tv2_;
  struct tvec tv3_;
  struct tvec tv4_;
  struct tvec tv5_;

  DECLARE_IDCREATE(TimerSystem);
};

inline TimerSystemInterface& GetTimerSystem() { return *(TimerSystem::GetObjectByID(0)); }
