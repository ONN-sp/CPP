#ifndef LOGGING_H
#define LOGGING_H

#include "../NonCopyAble.h" 
#include "Logstream.h"
#include <functional>
#include <string>
#include <memory>
#include <cstring>

namespace tiny_muduo{
//封装文件路径信息,提取文件名和长度
class SourceClass{
    public:
        SourceClass(const char* data)
        : data_(data),
          len_(static_cast<int>(std::strlen(data_))){
            const char* forward_slash = std::strrchr(data, '/'); // 寻找最后一个斜杠
            if(forward_slash){
                data_ = forward_slash + 1;// 最后一个斜杠后面就是文件名    获取文件名的指针
                len_ -= static_cast<int>((data_ - data));// 计算文件长度
            }
          }     

          const char* data_; //文件名指针
          int len_; // 文件名长度
};
//日志记录器
class Logger : NonCopyAble{
    public:
        enum class Level{
            DEBUG, // 调试信息级别
            INFO,  // 普通信息级别
            WARN,  // 警告信息级别
            ERROR, // 错误信息级别
            FATAL  // 致命错误信息级别
        };
        Logger(const char* file_, int line, Level level) : implement_(std::make_unique<Implement>(file_, line, level)){}
        ~Logger() = default;
        LogStream& stream( ){//获取日志流对象
          return (*implement_).stream();
        }
        using OutputFunc = std::function<void(const char*, int)>;//输出函数指针类型
        using FlushFunc = std::function<void()>;//刷新函数指针类型
        static void SetOutputFunc(OutputFunc);
        static void SetFlushFunc(FlushFunc);
    private:
        //日志实现类
        class Implement : public NonCopyAble{
            public:
                using Level = Logger::Level;
                Implement(SourceClass&&, int, Level);
                ~Implement();
                void FormattedTime();//格式化时间
                const char* GetLogLevel(int&) const ;//获取日志级别字符串

                void Finish(){//完成日志记录，输出文件名、行号等信息
                  stream_ << " - "
                  << GeneralTemplate(fileinfo_.data_, fileinfo_.len_) 
                  << ':' << line_ << '\n'; 
                }
                LogStream& stream(){//获取日志流对象
                  return stream_;
                }
            private:
                SourceClass fileinfo_; // 文件信息对象
                int line_; // 日志行号
                Level level_; // 日志级别
                LogStream stream_; // 日志流对象
        };
        std::unique_ptr<Implement> implement_; // 日志实现对象智能指针
};
}

// 返回当前的日志级别
tiny_muduo::Logger::Level LogLevel();
// 设置日志级别
void SetLogLevel(tiny_muduo::Logger::Level nowlevel);
// 错误转字符串
const char* ErrorToString(int err);
// 日志输出宏定义：根据日志级别判断是否输出日志   当使用宏,就会创建一个Logger对象,该对象在构造时会捕捉日志消息
#define LOG_DEBUG \
  if (LogLevel() <= tiny_muduo::Logger::Level::DEBUG) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::Level::DEBUG).stream()//如果当前日志级别比DEBUG小,才会执行DEBUG这个日志记录操作;如果比DEBUG大,那么就不会执行DEBUG这个日志记录(节省资源)
#define LOG_INFO \
  if (LogLevel() <= tiny_muduo::Logger::Level::INFO) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::Level::INFO).stream()
#define LOG_WARN \
  if (LogLevel() <= tiny_muduo::Logger::Level::WARN) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::Level::WARN).stream()
#define LOG_ERROR \
  if (LogLevel() <= tiny_muduo::Logger::Level::ERROR) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::Level::ERROR).stream()
#define LOG_FATAL \
  if (LogLevel() <= tiny_muduo::Logger::Level::FATAL) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::Level::FATAL).stream()
#endif