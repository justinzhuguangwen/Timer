// codecc
#include "lib_time_source.h"
#include "lib_crontab.h"
#include "lib_log.h"
#include "lib_time.h"

CTimeSource::CTimeSource() { Reset(); }

void CTimeSource::Clear() { memset(this, 0, sizeof(*this)); }

void CTimeSource::Reset() {
  Clear();
  UpdateTime();
}

void CTimeSource::UpdateTime(const timeval *tv) {
  struct timeval cur;
  if (tv == nullptr) {
    gettimeofday(&cur, NULL);
    tv = &cur;
  }

  if (TvCompare(tv, &real_time_) <= 0) {
    return;
  }
  time_t last_sec = time_.tv_sec;
  real_time_ = *tv;
  time_ = *tv;
  time_.tv_sec += time_delta_;
  if (last_sec != time_.tv_sec) {
    UpdatePeriodTime();
  }
}

int CTimeSource::SetTimeDelta(int delta) {
  if (delta == time_delta_) {
    return 0;
  }
  if (delta < time_delta_) {
    LogWarnM(LOGM_SYS, "invalid delta %d", delta);
    // 回溯的话需要清理一下
    Clear();
  }

  time_delta_ = delta;
  UpdateTime();
  return 0;
}

int CTimeSource::SetTime(time_t time) {
  int delta = time - real_time_.tv_sec;
  return SetTimeDelta(delta);
}

int CTimeSource::SetTime(const char *time_str) {
  time_t time = Stddatetime2TimeT(time_str);
  return SetTime(time);
}

int CTimeSource::SetTime(int year, int month, int day, int hour, int minute, int second) {
  time_t time = make_time(year, month, day, hour, minute, second);
  return SetTime(time);
}

void CTimeSource::UpdatePeriodTime() {
  time_t time = time_.tv_sec;
  if (time > day_end_time_) {
    day_end_time_ = GetNextCrontabTime("0 0 * * *", time);
  }
  if (time > day_end_am3_time_) {
    day_end_am3_time_ = GetNextCrontabTime("0 3 * * *", time);
  }

  if (time > week_end_time_) {
    week_end_time_ = GetNextCrontabTime("0 0 * * 1", time);
  }
  if (time > week_end_am3_time_) {
    week_end_am3_time_ = GetNextCrontabTime("0 3 * * 1", time);
  }

  if (time > month_end_time_) {
    month_end_time_ = GetNextCrontabTime("0 0 1 * *", time);
  }
  if (time > month_end_am3_time_) {
    month_end_am3_time_ = GetNextCrontabTime("0 3 1 * *", time);
  }
}

const timeval &GetRealTimeTv() {
  static timeval tv;
  gettimeofday(&tv, nullptr);
  return tv;
}

struct tm *GetTickLocaltime() {
  time_t t = GetTickTimeS();
  struct tm *_tm = localtime(&t);

  return _tm;
}

struct tm *GetRealLocaltime() {
  time_t t = GetRealTimeS();
  struct tm *_tm = localtime(&t);

  return _tm;
}
