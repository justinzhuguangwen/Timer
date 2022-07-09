// 时间源
#pragma once

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <cstdint>
#include "singleton.h"

const int SECOND_MS = 1000;     // 一秒的毫秒数
const int SECOND_US = 1000000;  // 一秒的微秒数

inline int64_t TimeValToMs(const struct timeval &tv) {
  return tv.tv_sec * SECOND_MS + tv.tv_usec / SECOND_MS;
}
inline int64_t TimeValToUs(const struct timeval &tv) { return tv.tv_sec * SECOND_US + tv.tv_usec; }

class CTimeSource {
 public:
  CTimeSource();
  void Reset();
  void UpdateTime(const struct timeval *tv = nullptr);
  int SetTimeDelta(int delta);
  int SetTime(time_t time);
  // YYYY-MM-DD-hh-mm-ss
  int SetTime(const char *time_str);
  int SetTime(int year, int month, int day, int hour, int minute, int second);
  time_t GetTimeDelta() { return time_delta_; }

  // delta time
  const timeval &GetTime() { return time_; }
  time_t GetTimeS() { return time_.tv_sec; }
  int64_t GetTimeMs() { return TimeValToMs(time_); }
  int64_t GetTimeUs() { return TimeValToUs(time_); }

  // real time
  const timeval &GetRealTime() { return real_time_; }
  time_t GetRealTimeS() { return real_time_.tv_sec; }
  int64_t GetRealTimeMs() { return TimeValToMs(real_time_); }
  int64_t GetRealTimeUs() { return TimeValToUs(real_time_); }

  // 这些时间都带偏移
  time_t GetDayEndTime() { return day_end_time_; }
  time_t GetDayEndAm3Time() { return day_end_am3_time_; }
  time_t GetWeekEndTime() { return week_end_time_; }
  time_t GetWeekEndAm3Time() { return week_end_am3_time_; }
  time_t GetMonthEndTime() { return month_end_time_; }
  time_t GetMonthEndAm3Time() { return month_end_am3_time_; }

 private:
  void Clear();
  void UpdatePeriodTime();

 private:
  struct timeval time_;
  struct timeval real_time_;
  time_t day_end_time_;
  time_t day_end_am3_time_;
  time_t week_end_time_;
  time_t week_end_am3_time_;
  time_t month_end_time_;
  time_t month_end_am3_time_;
  time_t time_delta_;  // 时间偏移（秒）
};

inline CTimeSource &GetTimeSource() { return Singleton<CTimeSource>::GetInstance(); }

inline time_t GetTimeDelta() { return GetTimeSource().GetTimeDelta(); }

// ticktime:
// 带time_delta_偏移的时间，在tick和update的时候更新，有毫秒级误差，一般10ms以内，游戏类普通逻辑可用
inline const timeval &GetTickTimeTv() { return GetTimeSource().GetTime(); }
inline time_t GetTickTimeS() { return GetTimeSource().GetTimeS(); }
inline int64_t GetTickTimeMs() { return GetTimeSource().GetTimeMs(); }
inline int64_t GetTickTimeUs() { return GetTimeSource().GetTimeUs(); }
struct tm *GetTickLocaltime();

// realticktime: 真实时间，在tick和update的时候更新，有毫秒级误差，一般10ms以内，和外部交互逻辑可用
inline const timeval &GetRealTickTimeTv() { return GetTimeSource().GetRealTime(); }
inline time_t GetRealTickTimeS() { return GetTimeSource().GetRealTimeS(); }
inline int64_t GetRealTickTimeMs() { return GetTimeSource().GetRealTimeMs(); }
inline int64_t GetRealTickTimeUs() { return GetTimeSource().GetRealTimeUs(); }

// realtime: 真实时间，gettimeodday，只有性能统计之类的才需要用这个时间
const timeval &GetRealTimeTv();
inline time_t GetRealTimeS() { return GetRealTimeTv().tv_sec; }
inline int64_t GetRealTimeMs() { return TimeValToMs(GetRealTimeTv()); }
inline int64_t GetRealTimeUs() { return TimeValToUs(GetRealTimeTv()); }
struct tm *GetRealLocaltime();
