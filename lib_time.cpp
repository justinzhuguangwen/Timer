// codecc
#include "lib_time.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "bitstring.h"
#include "lib_str.h"

const int LOGIC_DAY_TIME_OFFSET_3AM = 3;
const int LOGIC_DAY_OFFSET_HOUR = 3;      // 逻辑天与零点的向后偏移时间小时
#define LOGIC_NEWDAY_OFFSET_SECOND 14400  // 逻辑天与零点的向后偏移时间秒数――4小时

int IsMatchByDecimal(int index, int total_serial) {
  // [1,7]
  if (!(index >= 1 && index <= 7)) {
    return 0;
  }

  int iBase = pow(10, index - 1);
  if (total_serial / iBase % 10 == 1) {
    return 1;
  } else {
    return 0;
  }
}

// 相等返回1
int IsSameDay(time_t t1, time_t t2) {
  // 57600是东八区的偏移
  if (((t1)-57600) / 86400 == ((t2)-57600) / 86400) {
    return 1;
  }

  return 0;
}

int IsLogicSameDay(time_t t1, time_t t2) {
  return IsSameDayWithSplitHour(t1, t2, LOGIC_DAY_TIME_OFFSET_3AM);
}

// 相等返回1
int IsSameDayWithSplitHour(time_t t1, time_t t2, int split) {
  if (split < 0 || split > 23) {
    return 0;
  }
  return IsSameDay(t1 - (3600 * split), t2 - (3600 * split));
}

int SplitTimePart(int time_sec, int *day, int *hour, int *min) {
  if (day == NULL || hour == NULL || min == NULL) {
    return -1;
  }

  *day = time_sec / 86400;
  *hour = (time_sec - *day * 86400) / 3600;
  *min = (time_sec - *day * 86400 - *hour * 3600) / 60;

  return 0;
}

int GetLogicDayPassNum(time_t t1, time_t t2) {
  return GetDayPassNum_with_split_hour(t1, t2, LOGIC_DAY_TIME_OFFSET_3AM);
}

int GetDayPassNum_with_split_hour(time_t t1, time_t t2, int split) {
  if (split < 0 || split > 23) {
    return 0;
  }
  return GetDayPassNum(t1 - (3600 * split), t2 - (3600 * split));
}

int GetDayPassNum(time_t t1, time_t t2) { return (((t1)-57600) / 86400 - ((t2)-57600) / 86400); }
/*
// 8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  0   1   2   3   4   5   6   7
北京时间
// 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23
标准时间
*/
int GetTimeAddDays(time_t tBase, int iDays,
                   int split) {  // n天后的时间点，需要取以split为时间点的跨天的点
  int iOverRangeHour = (split - 8 + 24) % 24;  // 这个时间就是对应的标准时间上面的点

  return ((tBase - iOverRangeHour * 3600) / 86400 + iDays) * 86400 + iOverRangeHour * 3600;
}

int GetYear(time_t time) {
  struct tm *ptDate;

  ptDate = localtime(&time);
  if (NULL == ptDate) {
    return 0;
  }
  return ptDate->tm_year + 1900;
}

int GetMonth(time_t time) {
  struct tm *ptDate;

  ptDate = localtime(&time);
  if (NULL == ptDate) {
    return 0;
  }
  return ptDate->tm_mon + 1;
}

int GetDay(time_t time) {
  struct tm *ptDate;

  ptDate = localtime(&time);
  if (NULL == ptDate) {
    return 0;
  }
  return ptDate->tm_mday;
}

int64_t MsPass(struct timeval *tv1, struct timeval *tv2) {
  int64_t sec_diff = tv1->tv_sec - tv2->tv_sec;

  return sec_diff * SECOND_MS + (tv1->tv_usec - tv2->tv_usec) / SECOND_MS;
}

int64_t UsPass(struct timeval *tv1, struct timeval *tv2) {
  int64_t sec_diff = tv1->tv_sec - tv2->tv_sec;
  return sec_diff * SECOND_US + tv1->tv_usec - tv2->tv_usec;
}

struct timeval *TvAddMs(struct timeval *tv, int iMs) {
  if (0 == iMs) {
    return tv;
  }
  tv->tv_sec += iMs / SECOND_MS;
  tv->tv_usec += (iMs % SECOND_MS) * SECOND_MS;
  if (iMs > 0) {
    if (tv->tv_usec >= SECOND_US) {
      tv->tv_usec -= SECOND_US;
      tv->tv_sec++;
    }
  } else {  // < 0
    if (tv->tv_usec < 0) {
      tv->tv_usec += SECOND_US;
      tv->tv_sec--;
    }
  }
  return tv;
}
struct timeval *TvAddUs(struct timeval *tv, int iUs) {
  if (0 == iUs) {
    return tv;
  }
  tv->tv_sec += iUs / SECOND_US;
  tv->tv_usec += iUs % SECOND_US;
  if (iUs > 0) {
    if (tv->tv_usec >= SECOND_US) {
      tv->tv_usec -= SECOND_US;
      tv->tv_sec++;
    }
  } else {
    if (tv->tv_usec < 0) {
      tv->tv_usec += SECOND_US;
      tv->tv_sec--;
    }
  }
  return tv;
}

int TvCompare(const struct timeval *tv1, const struct timeval *tv2) {
  if (tv1->tv_sec != tv2->tv_sec) {
    return (tv1->tv_sec > tv2->tv_sec) ? 1 : -1;
  }
  if (tv1->tv_usec != tv2->tv_usec) {
    return (tv1->tv_usec > tv2->tv_usec) ? 1 : -1;
  }
  return 0;
}

/**
 *@ 根据小时和分钟重新获取当天的时间
 */
int GetNewDateTime(int hour, int minute) {
  struct tm *curr = GetTickLocaltime();
  if (NULL == curr) {
    return -1;
  }
  curr->tm_min = minute;
  curr->tm_hour = hour;
  curr->tm_sec = 0;

  return mktime(curr);
}

const char *CTimeStr::DateTime(time_t my_time) {
  if (my_time == time_) {
    return str_;
  }

  struct tm *tm_time = localtime(&my_time);
  if (tm_time == NULL) {
    return "";
  }

  time_ = my_time;
  if (tm_time->tm_year > 50) {
    SNPRINTF(str_, sizeof(str_), "%04d-%02d-%02d %02d:%02d:%02d", tm_time->tm_year + 1900,
             tm_time->tm_mon + 1, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min,
             tm_time->tm_sec);
  } else {
    SNPRINTF(str_, sizeof(str_), "%04d-%02d-%02d %02d:%02d:%02d", tm_time->tm_year + 2000,
             tm_time->tm_mon + 1, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min,
             tm_time->tm_sec);
  }
  return str_;
}

const char *CTimeStr::DateTimeNoYear(time_t my_time) {
  if (my_time == time_) {
    return str_;
  }

  struct tm *tm_time = localtime(&my_time);
  if (tm_time == NULL) {
    return "";
  }

  time_ = my_time;
  SNPRINTF(str_, sizeof(str_), "%02d-%02d %02d:%02d:%02d", tm_time->tm_mon + 1, tm_time->tm_mday,
           tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
  return str_;
}

const char *DateTimeStr(time_t mytime) {
  static CTimeStr time_str;
  return time_str.DateTime(mytime);
}

const char *CurDateTimeStr(void) {
  static CTimeStr time_str;
  return time_str.DateTime(GetTickTimeS());
}

const char *CurDateTimeStrNoYear(void) {
  static CTimeStr time_str;
  return time_str.DateTimeNoYear(GetTickTimeS());
}

// 检测日期字符串格式是否有效 0 无效
uint8_t CheckStddatetimeStrValid(const char *time) {
  if (time == NULL) {
    return 0;
  }

  if (strlen(time) != 19) {
    return 0;
  }

  char sTemp[20] = {'\0'};
  char s1[5] = {'\0'};
  STRNCPY(sTemp, time, sizeof(sTemp));

  SNPRINTF(s1, sizeof(s1), "%s", &sTemp[17]);  // 秒 0-60
  sTemp[16] = 0;
  if (atoi(s1) < 0 || atoi(s1) > 60) {
    return 0;
  }

  SNPRINTF(s1, sizeof(s1), "%s", &sTemp[14]);  // 分 0-59
  sTemp[13] = 0;
  if (atoi(s1) < 0 || atoi(s1) > 59) {
    return 0;
  }

  SNPRINTF(s1, sizeof(s1), &sTemp[11]);  // hour 0-23
  sTemp[10] = 0;
  if (atoi(s1) < 0 || atoi(s1) > 23) {
    return 0;
  }

  SNPRINTF(s1, sizeof(s1), "%s", &sTemp[8]);  // day  1-31
  sTemp[7] = 0;
  if (atoi(s1) < 1 || atoi(s1) > 31) {
    return 0;
  }

  SNPRINTF(s1, sizeof(s1), "%s", &sTemp[5]);  // month  1-12
  sTemp[4] = 0;
  if (atoi(s1) < 1 || atoi(s1) > 12) {
    return 0;
  }

  if (atoi(sTemp) < 1900) {  // year
    return 0;
  }

  return 1;
}

time_t StddateToTimeT(const char *time) {
  char sTemp[20], s1[5];
  struct tm tmTime = {};
  time_t lTime;

  if (strlen(time) != 10) {
    return 0;
  }
  STRNCPY(sTemp, time, sizeof(sTemp));

  SNPRINTF(s1, sizeof(s1), "%s", &sTemp[8]);
  sTemp[7] = 0;
  tmTime.tm_mday = atoi(s1);

  SNPRINTF(s1, sizeof(s1), "%s", &sTemp[5]);
  sTemp[4] = 0;
  tmTime.tm_mon = atoi(s1) - 1;

  tmTime.tm_year = atoi(sTemp) - 1900;

  tmTime.tm_isdst = 0;  // 标准时间，非夏令时
  lTime = mktime(&tmTime);

  return lTime;
}

time_t Stddatetime2TimeT(const char *time) {
  if (time == nullptr) {
    return -1;
  }

  struct tm lmt;
  int n;
  n = sscanf(time, "%04d-%02d-%02d-%02d-%02d-%02d", &lmt.tm_year, &lmt.tm_mon, &lmt.tm_mday,
             &lmt.tm_hour, &lmt.tm_min, &lmt.tm_sec);
  if (6 != n) {
    return -1;
  }

  lmt.tm_year -= 1900;
  lmt.tm_mon -= 1;
  lmt.tm_isdst = 0;
  return mktime(&lmt);
}

int IsLeapYear(int tm_year) {
  int iYear = tm_year;
  if (tm_year > 50) {
    iYear += 1900;
  } else {
    iYear += 2000;
  }

  /* Nonzero if YEAR is a leap year (every 4 years, except every 100th isn't, and every 400th is).
   */
  if (iYear % 4 == 0 && (iYear % 100 != 0 || iYear % 400 == 0)) {
    return 1;
  }
  return 0;
}
int GetDayNumByMonth(int tm_year, int tm_mon) {
  int iMonthDayNum = 0;
  switch (tm_mon + 1) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      iMonthDayNum = 31;
      break;
    case 2:
      iMonthDayNum = 28;
      if (1 == IsLeapYear(tm_year)) {  // 闰
        iMonthDayNum = 29;
      }
      break;
    default:
      iMonthDayNum = 30;
      break;
  }
  return iMonthDayNum;
}
time_t MktimeByStrtime(char time[]) {
  struct tm stTmOut;
  strptime(time, "%Y%m%d%H%M%S", &stTmOut);
  return mktime(&stTmOut);
}

time_t GetTimet4DayEnd(time_t time, int next_day_sum) {
  time += next_day_sum * DAY_SECOND;
  // andywxwang 这里不做虚拟时间函数替换，因为tInputTime参数已经是偏移过的时间
  struct tm *stInputTm = localtime(&time);
  if (stInputTm == NULL) {
    return 0;
  }
  int iYear = stInputTm->tm_year;
  int iMon = stInputTm->tm_mon;
  int imDay = stInputTm->tm_mday;
  if (iYear > 50) {
    iYear += 1900;
  } else {
    iYear += 2000;
  }
  char strTime[32];
  SNPRINTF(strTime, sizeof(strTime), "%d%02d%02d235959", iYear, iMon + 1, imDay);
  return MktimeByStrtime(strTime) + 1;  // 再加1秒
}
time_t GetTimet4LogicdayEnd(time_t time, int next_day_sum) {
  time += next_day_sum * DAY_SECOND;
  time_t logicDayEnd = GetTimet4DayEnd(time, 0) + LOGIC_DAY_TIME_OFFSET_3AM * HOUR_SECOND;
  if (logicDayEnd > time + DAY_SECOND) {
    logicDayEnd -= DAY_SECOND;
  }
  return logicDayEnd;
}

time_t GetTimet4WeekEnd(struct tm *tm, int next_week_sum) {
  if (NULL == tm) {
    return -1;
  }
  int iYear = tm->tm_year;
  int iMon = tm->tm_mon;
  int imDay = tm->tm_mday;
  int iAddDay = 7 * next_week_sum;

#ifdef _WEEK_SINCE_SUNDAY_  // 周按照从周日算起到周六
  iAddDay += (6 - tm->tm_wday);
#else  // else 按照从周一到周日计算
  if (tm->tm_wday != 0) {
    iAddDay += (7 - tm->tm_wday);
  }
#endif

  while (iAddDay > 0) {
    int iDayNum = GetDayNumByMonth(iYear, iMon);
    int iLeftDay = iDayNum - imDay;
    if (iAddDay <= iLeftDay) {
      imDay += iAddDay;
      break;
    } else {
      iAddDay -= (iLeftDay + 1);
      imDay = 1;  // 到下个月1日
      int iTmpMon = iMon + 1;
      iMon = iTmpMon % 12;
      iYear += iTmpMon / 12;
    }
  }
  if (iYear > 50) {
    iYear += 1900;
  } else {
    iYear += 2000;
  }
  char strTime[32];
  SNPRINTF(strTime, sizeof(strTime), "%d%02d%02d235959", iYear, iMon + 1, imDay);
  return MktimeByStrtime(strTime) + 1;  // 再加1秒
}
time_t GetTimet4MonthEnd(struct tm *tm, int next_month_sum) {
  if (NULL == tm) {
    return -1;
  }
  int iYear = tm->tm_year;
  int iTmpMon = tm->tm_mon;
  iTmpMon += next_month_sum;
  int iMon = iTmpMon % 12;
  iYear += iTmpMon / 12;
  int iDayNum = GetDayNumByMonth(iYear, iMon);
  if (iYear > 50) {
    iYear += 1900;
  } else {
    iYear += 2000;
  }
  char strTime[32];
  SNPRINTF(strTime, sizeof(strTime), "%d%02d%02d235959", iYear, iMon + 1, iDayNum);
  return MktimeByStrtime(strTime) + 1;  // 再加1秒
}
time_t GetTimet4YearEnd(struct tm *tm, int next_year_num) {
  if (NULL == tm) {
    return -1;
  }

  int iYear = tm->tm_year + next_year_num;
  if (iYear > 50) {
    iYear += 1900;
  } else {
    iYear += 2000;
  }
  char strTime[32];
  SNPRINTF(strTime, sizeof(strTime), "%d1231235959", iYear);
  return MktimeByStrtime(strTime) + 1;  // 再加1秒
}

/**
 * 获取时间
 */
time_t make_time(int year, int month, int day, int hour, int minute, int second) {
  struct tm tmTime;

  tmTime.tm_year = year - 1900;
  tmTime.tm_mon = month - 1;
  tmTime.tm_mday = day;
  tmTime.tm_hour = hour;
  tmTime.tm_min = minute;
  tmTime.tm_sec = second;

  tmTime.tm_isdst = 0;  // 标准时间，非夏令时

  return mktime(&tmTime);
}

// 获取时间，会检查参数有效性
time_t MakeTimeCheckParam(int year, char month, char day, char hour, char minute) {
  if (year < 1900 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 60 ||
      minute < 0 || minute > 60) {
    return -1;
  }

  // 该月最大天数
  int max_day = GetDayNumByMonth(year - 1900, month - 1);

  // 不能超过月份最大天数
  if (day > max_day) {
    day = max_day;
  }

  return make_time(year, month, day, hour, minute, 0);
}

// END erisenxu 从字符串中获取日期和时间 2014-01-21 18:00

// 虚拟时间轴相关操作
#if 1

time_t UtcAddTime(time_t time, int year, int month, int day, int hour, int minute, int second) {
  time += second;
  time += minute * MIN_SECOND;
  time += hour * HOUR_SECOND;
  time += day * DAY_SECOND;

  // localtime 性能不好，不必要的话就不调用了。
  if (year != 0 || month != 0) {
    struct tm *_tm = localtime(&time);
    if (_tm == nullptr) {
      return time;
    }

    _tm->tm_year += year;
    _tm->tm_mon += month;

    int dYear = (_tm->tm_mon > 0) ? _tm->tm_mon % 12 : _tm->tm_mon % 12 - 1;

    _tm->tm_year += dYear;
    _tm->tm_mon -= dYear * 12;

    time = mktime(_tm);
  }

  return time;
}
#endif
