#ifndef TINY_MUDUO_TIMESTAMP_H_
#define TINY_MUDUO_TIMESTAMP_H_

//使用Linux的计时和获取时间的函数,不使用std::chrono库
// #include <stdio.h>
// #include <sys/time.h>
// #include <string>
// #include "noncopyable.h"

// const int kMicrosecond2Second = 1000*1000;//us->s
// class Timestamp {
//     public:
//         Timestamp():micro_seconds_(0){}
//         explicit Timestamp(int64_t micro_seconds):micro_seconds_(micro_seconds) {}

//         bool operator<(const Timestamp& rhs) const {
//             return micro_seconds_ < rhs.microseconds();
//         }

//         bool operator==(const Timestamp& rhs) const {
//             return micro_seconds_ == rhs.microseconds();
//         }

//         std::string ToFormattedDefaultLogString() const {
//             char buf[64] = {0};
//             time_t seconds = static_cast<time_t>(micro_seconds_ / kMicrosecond2Second);
//             struct tm tm_time;
//             localtime_r(&seconds, &tm_time);
//             snprintf(buf, sizeof(buf), "%4d%02d%02d_%02d%02d%02d",
//                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
//                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
//             return buf;
//         }

//         std::string ToFormattedString() const {
//             char buf[64] = {0};
//             time_t seconds = static_cast<time_t>(micro_seconds_ / kMicrosecond2Second);
//             struct tm tm_time;
//             localtime_r(&seconds, &tm_time);
//             int micro_seconds = static_cast<int>(micro_seconds_ % kMicrosecond2Second);
//             snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
//                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
//                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, micro_seconds);
//             return buf;
//         }

//         int64_t microseconds() const { return micro_seconds_; } //ms

//         static Timestamp Now();
//         static Timestamp AddTime(const Timestamp& timestamp, double add_seconds);

//     private:
//         int64_t micro_seconds_;
//     };

// //获取当前时间
// inline Timestamp Timestamp::Now() {
//     struct timeval time;
//     gettimeofday(&time,NULL);
//     return Timestamp(time.tv_sec * kMicrosecond2Second + time.tv_usec); 
// }

// //添加时间
// inline Timestamp Timestamp::AddTime(const Timestamp& timestamp, double add_seconds) {
//     int64_t add_microseconds = static_cast<int64_t>(add_seconds) * kMicrosecond2Second;   
//     return Timestamp(timestamp.microseconds() + add_microseconds);
// }

//使用C++11提供的std::chrono时间库
#include <chrono>
#include <string>
#include <ctime>
#include <iomanip>//std::put_time
#include <sstream>//.str()

const int kMicrosecond2Second = 1000*1000;//us->s
class Timestamp {
  public:
    using Clock = std::chrono::system_clock;//使用系统时钟
    using TimePoint = std::chrono::time_point<Clock>;//定义时间点类型(时间点<=>时刻)
    using Microseconds = std::chrono::microseconds;//定义微秒类型

    Timestamp():micro_seconds_(Clock::now()){}
    explicit Timestamp(TimePoint micro_seconds_):micro_seconds_(micro_seconds_){}

    bool operator<(const Timestamp& rhs) const {
      return micro_seconds_ < rhs.micro_seconds_;
    }

    bool operator==(const Timestamp& rhs) const {
      return micro_seconds_ == rhs.micro_seconds_;
    }

    std::string ToFormattedDefaultLogString() const {
      return FormatTime("%Y%m%d_%H%M%S");
    }

    std::string ToFormattedString() const {
      auto duration = micro_seconds_.time_since_epoch();//获取当前时间点距离时钟起点的持续时间
      auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);//将持续时间转换为秒值
      auto microseconds = std::chrono::duration_cast<Microseconds>(duration - seconds);////将持续时间转换为秒后剩的微秒值
      return FormatTime("%Y%m%d %H:%M:%S") + "." + std::to_string(microseconds.count());//%Y:四位数的年份;%m:两位数的月份;%d:两位数的日期;%H:24小时制的小时数;%M:分钟数;%S:秒数
    }

    int64_t microseconds()const{//返回当前微秒数
      return std::chrono::duration_cast<Microseconds>(micro_seconds_.time_since_epoch()).count();
    }

    int64_t seconds()const{//返回当前秒数
      return std::chrono::duration_cast<std::chrono::seconds>(micro_seconds_.time_since_epoch()).count();
    }

    static Timestamp Now(){//返回当前时间
      return Timestamp(Clock::now());
    }
 
    static Timestamp AddTime(const Timestamp& timestamp, double add_seconds){//返回增加指定秒数后的时间
      auto duration = Microseconds(static_cast<int64_t>(add_seconds * kMicrosecond2Second));//将指定的秒数转换为微秒数
      return Timestamp(timestamp.micro_seconds_ + duration);//<chrono>定义了std::chrono::time_point时间点类型和std::chrono::duration持续时间的直接相加操作
    }
  private:
    TimePoint micro_seconds_;//时间的微妙表示
    std::string FormatTime(const char* format) const {//将micro_seconds_表示的时间点格式化为指定格式的字符串,并返回该字符串
      auto time_t = Clock::to_time_t(micro_seconds_);//将micro_seconds_转换为标准C时间(time_t),因为localtime_r要用C时间
      std::tm tm_time;
      localtime_r(&time_t, &tm_time); // POSIX,thread safe. 将时间戳time_t转换为本地时间的tm结构体(tm_time)
      std::ostringstream oss;
      oss << std::put_time(&tm_time, format);//将tm_time结构体按照指定的format格式输出
      return oss.str();//返回格式化后的字符串
    }
};
#endif