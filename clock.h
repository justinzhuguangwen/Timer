// Copyright (C) 2018 simon@qchen.fun. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>
#include <string>

class Clock {
 public:
  // Get current unix time in milliseconds
  static int64_t CurrentTimeMillis();

  // Get current unix time in milliseconds
  static int64_t SystemTimeMillis();

  // Get current time in string format
  static std::string CurrentTimeString(int64_t timepoint);

  // Get current tick count, in nanoseconds
  static int64_t GetNowTickCount();
  static void TimeFly(int64_t ms);
  static void TimeReset();

 private:
  static int64_t clock_offset_;  // offset time of logic clock
};
