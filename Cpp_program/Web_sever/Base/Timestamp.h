#ifndef TINY_MUDUO_TIMESTAMP_H_
#define TINY_MUDUO_TIMESTAMP_H_

#include <chrono>
#include <string>
#include <ctime>

static const int kMicrosecond2Second = 1000*1000;//us->s 
namespace tiny_muduo{
class Timestamp {
  public:
    using Clock = std::chrono::system_clock;//使用系统时钟
    using TimePoint = std::chrono::time_point<Clock>;//定义时间点类型(时间点<=>时刻)
    using Microseconds = std::chrono::microseconds;//定义微秒类型

    Timestamp();
    explicit Timestamp(TimePoint);
    bool operator<(const Timestamp& rhs) const {
      return micro_seconds_ < rhs.micro_seconds_;
    }
    bool operator==(const Timestamp& rhs) const {
      return micro_seconds_ == rhs.micro_seconds_;
    }
    std::string ToFormattedDefaultLogString() const;
    std::string ToFormattedString() const;
    static Timestamp Now();
    static Timestamp AddTime(const Timestamp&, double);
    int64_t microseconds() const ;
    int64_t seconds() const ;
    static std::chrono::system_clock::time_point timestampToTimePoint(const Timestamp&);
    static Timestamp timepointToTimestamp(const TimePoint&);
    std::string FormatTime(const char* format) const;//将micro_seconds_表示的时间点格式化为指定格式的字符串,并返回该字符串
  private:
    TimePoint micro_seconds_;//时间的微妙表示 
};
}
#endif