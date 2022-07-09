/**
 * 时间相关操作函数。
 */
#pragma once

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <cstdint>
#include "lib_crontab.h"
#include "lib_time_source.h"

const int MS_FOR_HALF_DAY = 43200000;  // 半天的最大毫秒数

const int MIN_SECOND = 60;                  // 一分钟的秒数
const int HOUR_SECOND = (60 * MIN_SECOND);  // 一小时的秒数
const int DAY_SECOND = (24 * HOUR_SECOND);  // 一天的秒数
const int WEEK_SECOND = (7 * DAY_SECOND);   // 一周的秒数
const int YEAR_MONTH = (12);                // 一年的月数

const int TIME_ZONE = 8;  // 时区：+8时区

/**
 * 计算tv1-tv2的毫秒数。
 */
int64_t MsPass(struct timeval *tv1, struct timeval *tv2);

/**
 * 计算tv1-tv2的微秒数
 */
int64_t UsPass(struct timeval *tv1, struct timeval *tv2);
/**
 * timeval结构加上若干毫秒。
 *
 */
struct timeval *TvAddMs(struct timeval *tv, int ms);
/**
 * timeval结构加上若干u秒。
 *
 */
struct timeval *TvAddUs(struct timeval *tv, int us);

/**
 * 比较tv1和tv2大小
 * @param tv1 第一个时间
 * @param tv2 第二个时间
 * @return >0表示tv1>tv2，<0表示tv1<tv2，=0表示tv1==tv2
 */
int TvCompare(const struct timeval *tv1, const struct timeval *tv2);

// 时间转换成字符串，带个cache
class CTimeStr {
 public:
  CTimeStr() { memset(this, 0, sizeof(CTimeStr)); }
  // YYYY-MM-DD hh:mm:ss格式时间
  const char *DateTime(time_t my_time);

  // MM-DD hh:mm:ss格式时间
  const char *DateTimeNoYear(time_t my_time);

 private:
  time_t time_;
  char str_[50];
};

/**
 * @param mytime time_t时间
 * @return YYYY-MM-DD hh:mm:ss格式时间
 */
const char *DateTimeStr(time_t mytime);

/**
 * @return 当前的YYYY-MM-DD hh:mm:ss格式时间
 */
const char *CurDateTimeStr(void);

/**
 * @return 当前的MM-DD hh:mm:ss格式时间
 */
const char *CurDateTimeStrNoYear(void);

int IsLeapYear(int tm_year);
int GetDayNumByMonth(int tm_year, int tm_mon);
time_t MktimeByStrtime(char time[]);
time_t GetTimet4DayEnd(time_t input_time, int next_day_sum);
time_t GetTimet4WeekEnd(struct tm *tm, int next_week_sum);
time_t GetTimet4MonthEnd(struct tm *tm, int next_month_sum);
time_t GetTimet4YearEnd(struct tm *tm, int next_year_num);
time_t GetTimet4LogicdayEnd(time_t input_time, int next_day_sum);

// index是否在十进制的totalserial中，例如index=2表示右起十进制的第二位是1，即：xxxxx1x
// 一般用作一周内的配置，活动整合表中用， index=[1,7]
int IsMatchByDecimal(int index, int total_serial);

int IsSameDay(time_t t1, time_t t2);       // 相等返回1
int IsLogicSameDay(time_t t1, time_t t2);  // 相等返回1，AM3是逻辑跨天时间点
int IsSameDayWithSplitHour(time_t t1, time_t t2, int split);  // 相等返回1

int SplitTimePart(int time_sec, int *day, int *hour, int *min);

int GetDayPassNum(time_t t1, time_t t2);
int GetTimeAddDays(time_t base, int days, int split);  // n天后的时间点，需要取跨天的点

int GetDayPassNum_with_split_hour(time_t t1, time_t t2, int split);
int GetLogicDayPassNum(time_t t1, time_t t2);  // 3点
// 根据时间活动年月日
int GetYear(time_t time);
int GetMonth(time_t time);
int GetDay(time_t time);
/**
 *@Desc 根据新的小时和分钟数获得新的当天时间
 */
int GetNewDateTime(int hour, int minute);

// 检测日期字符串格式是否有效 0 无效
uint8_t CheckStddatetimeStrValid(const char *time);

// 字符串类型的时间转 time_t
time_t StddateToTimeT(const char *time);
time_t Stddatetime2TimeT(const char *time);

/**
 * 获取时间
 */
time_t make_time(int year, int month, int day, int hour, int minute, int second);

time_t MakeTimeCheckParam(int year, char month, char day, char hour, char minute);

time_t UtcAddTime(time_t time, int year, int month, int day, int hour, int minute, int second);

class CCostTime {
 public:
  int64_t start_;

 public:
  CCostTime() : start_(GetRealTimeMs()) {}

  int64_t DiffMsConst() const { return GetRealTimeMs() - start_; }

  int64_t DiffMs(bool reset = false) {
    int64_t end = GetRealTimeMs();
    int64_t ret = end - start_;
    if (reset) {
      start_ = end;
    }
    return ret;
  }
};

class TimeUnit {
 public:
  TimeUnit() = default;
  int64_t Value() { return value_; }

 protected:
  int64_t value_;
};

class Millis : public TimeUnit {
 public:
  Millis(int64_t value) { value_ = value; }
};

class Sec : public TimeUnit {
 public:
  Sec(int64_t value) { value_ = value * 1000; }
};

class Min : public TimeUnit {
 public:
  Min(int64_t value) { value_ = value * MIN_SECOND * 1000; }
};

class Hour : public TimeUnit {
 public:
  Hour(int64_t value) { value_ = value * HOUR_SECOND * 1000; }
};

class Day : public TimeUnit {
 public:
  Day(int64_t value) { value_ = value * DAY_SECOND * 1000; }
};

class Week : public TimeUnit {
 public:
  Week(int64_t value) { value_ = value * WEEK_SECOND * 1000; }
};

/* 辅助类，用来方便生成毫秒时间, 且阅读性较好
 * @examples:{Millis(100)}
 * {Sec(10), Millis(50)}
 * {Min(1), 0, Millis(50)}
 * {Hour(1), Min(1), Sec(10), Millis(50)}
 * {Millis(-100)}, {Sec(-1)}
 */
class TimeHelper {
 public:
  TimeHelper(Millis millis = {0}) { millis_ = millis.Value(); }
  TimeHelper(Sec sec, Millis millis = {0}) { millis_ = sec.Value() + millis.Value(); }
  TimeHelper(Min min, Sec sec = {0}, Millis millis = {0}) {
    millis_ = min.Value() + sec.Value() + millis.Value();
  }
  TimeHelper(Hour hour, Min min = {0}, Sec sec = {0}, Millis millis = {0}) {
    millis_ = hour.Value() + min.Value() + sec.Value() + millis.Value();
  }
  TimeHelper(Day day, Hour hour = {0}, Min min = {0}, Sec sec = {0}, Millis millis = {0}) {
    millis_ = day.Value() + hour.Value() + min.Value() + sec.Value() + millis.Value();
  }

  // TODO:"2011-02-18 23:12:34"

  // millisecond
  int64_t GetMillis() { return millis_; }

 private:
  int64_t millis_ = 0;
};
