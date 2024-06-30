#ifndef ASYNCLOGGING_H
#define ASYNCLOGGING_H

#include <chrono>
#include "../NonCopyAble.h"
#include "logstream.h"
#include <memory>
#include <vector>
#include "logfile.h"
#include "../MutexLock.h"
#include "../condition.h"
#include "../thread.h"
#include "../latch.h"

namespace tiny_muduo{
    static const std::chrono::seconds kBufferWriteTimeout (3);//3秒前端和后端日志线程交换
    static const int64_t kSingleFileMaxSize = 1024*1024*1024;//写入缓冲区的单个日志文件的最大大小
    class AsyncLogging : public NonCopyAble{
        public:
            using Buffer = FixedBuffer<kLargeSize>;
            using BufferPtr = std::unique_ptr<Buffer>;
            using BufferVector = std::vector<BufferPtr>;
            using LogFilePtr = std::unique_ptr<LogFile>;
            AsyncLogging(const std::string&, long);
            ~AsyncLogging();
            void Stop();// 停止异步日志记录线程
            void StartAsyncLogging();// 开始异步日志记录
            void Append(const char*, int);// 向日志系统追加日志数据
            void AsyncFlush(LogFilePtr);// 强制刷新日志
            void ThreadFunc();// 日志线程的主函数
        private:
            bool running_;
            const std::string filepath_;
            long rollSize_;
            MutexLock mutex_;// 互斥锁，保护共享的前端缓冲区
            Condition cond_;// 用于前端线程满了或者3秒到了而通知后端线程可以往本地文件写了
            Latch latch_;
            Thread thread_;// 日志线程对象
            //下面是前端线程的两个缓冲区
            BufferPtr current_;//当前业务线程(前端)的当前缓冲区
            BufferPtr next_;//前端的备用缓冲区
            BufferVector bufferToWrite_;//日志线程中要写入本地文件的缓冲区队列
    };
}


#endif