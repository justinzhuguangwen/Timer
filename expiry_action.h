// @brief
// Expiry action定义了超时行为基类
//  @author justinzhu
//  @date 2022年6月28日18:16:35

#pragma once

#include <functional>
#include "comm_base.h"
#include "singleton.h"

class ExpiryAction {
 public:
  virtual ~ExpiryAction() = default;
  virtual void OnExpiry(int32_t timer_globalid, int64_t user_data) = 0;
};

// example:
// class ExpriyActionTest : public ExpiryAction {
// public:
//  virtual ~ExpriyActionTest(){};
//  void OnExpiry(int32_t timer_globalid, int64_t data) override {
//    Timer* timer =
//        dynamic_cast<Timer*>(CIDRuntimeClass::GetObjFromGlobalID(timer_globalid, EOT_OBJ_TIMER));
//    printf("callback timer:%s now:(%s, %ld), data:%lu\n", timer->DebugString().c_str(),
//           Clock::CurrentTimeString(0).c_str(), Clock::CurrentTimeMillis(), data);
//    int64_t diff = Clock::CurrentTimeMillis() - (int64_t)timer->Expires();
//    if (abs(diff) > max_diff)
//      max_diff = abs(diff);
//    num_++;
//  }
//
// public:
//  int32_t num_ = 0;
//};
