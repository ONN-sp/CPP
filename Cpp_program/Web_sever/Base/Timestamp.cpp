#include "Timestamp.h"
#include <iomanip>//std::put_time
#include <sstream>//.str()

using namespace tiny_muduo;

Timestamp::Timestamp():micro_seconds_(Clock::now()){}
Timestamp::Timestamp(TimePoint micro_seconds_):micro_seconds_(micro_seconds_){}

std::string Timestamp::ToFormattedDefaultLogString() const {
    return Timestamp::FormatTime("%Y%m%d_%H%M%S");
}

std::string Timestamp::ToFormattedString() const {
    auto duration = micro_seconds_.time_since_epoch();//获取当前时间点距离时钟起点的持续时间
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);//将持续时间转换为秒值
    auto microseconds = std::chrono::duration_cast<Microseconds>(duration - seconds);////将持续时间转换为秒后剩的微秒值
    return Timestamp::FormatTime("%Y%m%d %H:%M:%S") + "." + std::to_string(microseconds.count());//%Y:四位数的年份;%m:两位数的月份;%d:两位数的日期;%H:24小时制的小时数;%M:分钟数;%S:秒数
}

int64_t Timestamp::microseconds()const{//返回当前微秒数
    return std::chrono::duration_cast<Microseconds>(micro_seconds_.time_since_epoch()).count();
}

int64_t Timestamp::seconds()const{//返回当前秒数
    return std::chrono::duration_cast<std::chrono::seconds>(micro_seconds_.time_since_epoch()).count();
}

Timestamp Timestamp::Now(){//返回当前时间
    return Timestamp(Clock::now());
}

Timestamp Timestamp::AddTime(const Timestamp& timestamp, double add_seconds){//返回增加指定秒数后的时间
    auto duration = Microseconds(static_cast<int64_t>(add_seconds * kMicrosecond2Second));//将指定的秒数转换为微秒数
    return Timestamp(timestamp.micro_seconds_ + duration);//<chrono>定义了std::chrono::time_point时间点类型和std::chrono::duration持续时间的直接相加操作
}

std::string Timestamp::FormatTime(const char* format) const {//将micro_seconds_表示的时间点格式化为指定格式的字符串,并返回该字符串
      auto time_t = Clock::to_time_t(micro_seconds_);//将micro_seconds_转换为标准C时间(time_t),因为localtime_r要用C时间
      std::tm tm_time;
      localtime_r(&time_t, &tm_time); // POSIX,thread safe. 将时间戳time_t转换为本地时间的tm结构体(tm_time)
      std::ostringstream oss;
      oss << std::put_time(&tm_time, format);//将tm_time结构体按照指定的format格式输出
      return oss.str();//返回格式化后的字符串
    }
