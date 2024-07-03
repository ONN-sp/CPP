#include "Logging.h"
#include "../CurrentThread.h"
#include <utility>
#include <cerrno>
#include <iomanip>
#include "../Timestamp.h"
#include <iostream>

namespace tiny_muduo{
    //使用线程局部存储(__thread)来保存线程特定的数据
    __thread time_t t_last_second;// 上次记录的时间戳（秒）
    __thread char t_time[64];// 缓存时间字符串
    __thread const int t_formattedtimeLength = 18;// 格式化时间字符串的长度
    __thread char t_errorbuf[512];// 用于存储错误信息的缓冲区
}

using namespace tiny_muduo;

const char* ErrorToString(int err){// 错误码->字符串
    return strerror_r(err, t_errorbuf, sizeof(t_errorbuf));
}

// 默认的日志输出函数：输出到标准输出
static void DefaultOutput(const char* data, int len) {
    fwrite(data, sizeof(char), len, stdout); // 将数据写到标准输出
}

// 默认的刷新函数：刷新标准输出
static void DefaultFlush() {
    fflush(stdout); // 刷新标准输出缓冲区
}

Logger::OutputFunc g_output = DefaultOutput;
Logger::FlushFunc g_flush = DefaultFlush;
Logger::Level g_level = Logger::Level::INFO;//默认日志为INFO级别

//自定义输出函数
void Logger::SetOutputFunc(Logger::OutputFunc func){
    g_output = func;
}

//自定义刷新函数
void Logger::SetFlushFunc(Logger::FlushFunc func){
    g_flush = func;
}

//获取当前日志级别
Logger::Level LogLevel(){
    return g_level;
}

//设置新的日志级别
void SetLogLevel(Logger::Level new_level) { 
    g_level = new_level;
}

Logger::Implement::Implement(SourceClass&& source, int line, Level level)
                    : fileinfo_(std::move(source)),
                      line_(line), // 日志记录的行号
                      level_(level){
                        FormattedTime(); // 格式化当前时间
                        tid(); // 记录当前线程的ID  用于构成日志消息的一部分
                        int kLogLevelStringLength = 0; // 日志级别字符串的长度
                        stream_ << GeneralTemplate(t_formattedTid, t_formattedTidLength);//写入长度比正确长度大了就会乱码
                        const char* temp = GetLogLevel(kLogLevelStringLength);// ??? 不清楚为什么直接传入GetLogLevel(kLogLevelStringLength)函数无法在GeneralTemplate的调用中正确返回kLogLevelStringLength的值
                        stream_ << GeneralTemplate(temp, kLogLevelStringLength);//写入长度比正确长度大了就会乱码 
                      }

Logger::Implement::~Implement(){
    Finish(); //完成日志记录，输出文件名、行号等信息
    const LogStream::Buffer& buffer = stream_.buffer(); // 获取日志缓冲区
    g_output(buffer.data(), buffer.len()); // 调用输出函数
}

// 根据日志级别返回相应的字符串
const char* Logger::Implement::GetLogLevel(int& kLogLevelStringLength) const {
    switch(level_){
        case Logger::Level::DEBUG:
			kLogLevelStringLength = 6;
            return "DEBUG ";
        case Logger::Level::INFO:
			kLogLevelStringLength = 5;
            return "INFO ";
        case Logger::Level::WARN:
			kLogLevelStringLength = 5;
            return "WARN ";
        case Logger::Level::ERROR:
			kLogLevelStringLength = 6;
            return "ERROR ";
        case Logger::Level::FATAL:
			kLogLevelStringLength = 6;
            return "FATAL ";
    }
    return nullptr;
}

// 格式化时间
void Logger::Implement::FormattedTime(){
    Timestamp now = Timestamp::Now();
    time_t seconds = static_cast<time_t>(now.microseconds() / kMicrosecond2Second);
    int microseconds = static_cast<int>(now.microseconds() % kMicrosecond2Second);
    if(t_last_second != seconds){
        struct tm tm_time;
        localtime_r(&seconds, &tm_time);
        std::ostringstream oss;
        oss << std::put_time(&tm_time, "%Y%m%d %H:%M:%S");
        std::string str  = oss.str().c_str();
        std::strcpy(t_time, (str+".").c_str());
        t_last_second = seconds;
    }
    char buf[32] = {0};// 格式化微秒部分   和Timestamp中类似
    int microlen = snprintf(buf, sizeof(buf), "%06d ", microseconds);
    // 向日志流中添加时间、线程ID和日志级别
    stream_ << GeneralTemplate(t_time, t_formattedtimeLength);
    stream_ << GeneralTemplate(buf, microlen);
}




