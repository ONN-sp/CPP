#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include "Latch.h"
#include <functional>
#include "NonCopyAble.h"
#include <sys/prctl.h>
#include <string>
#include "CurrentThread.h"

namespace tiny_muduo
{
    class Thread : public NonCopyAble {
        public:
            using ThreadFunc = std::function<void()>;
            Thread(const ThreadFunc&, const std::string& name = std::string());// name参数提供了默认值
            ~Thread();
            //启动线程
            void StartThread();
            //等待线程结束
            void Join();
            void ThreadRun();
        private:
            std::thread thread_;
            ThreadFunc func_;
            Latch latch_;//latch确保线程正确启动 
            std::string thread_name_;//线程名
            bool started_;//标志线程是否已经启动
    };

    struct ThreadData{
        using ThreadFunc = tiny_muduo::Thread::ThreadFunc;
        ThreadFunc func_;
        Latch* latch_;
        std::string thread_name_;
        ThreadData(const ThreadFunc& func, Latch* latch, const std::string& name)
                : func_(func),
                  latch_(latch),
                  thread_name_(name)
                  {}
        void RunOneFunc(){
            //线程启动时,先递减Latch的计数
            latch_->CountDown();//减1表示成功启动一个线程
            latch_ = nullptr;
            //为当前线程生成一个唯一的名称，并设置该名称
            char buf[32] = {0};
            snprintf(buf, sizeof(buf), "%d", (tid()));//生成当前线程ID并转换为字符串
            ::prctl(PR_SET_NAME, thread_name_.size() == 0 ? ("WorkThread " + std::string(buf)).data() : thread_name_.data());//设置线程名称(人为取了一部分名,为了便于调试)
            func_();
        }
    };
} 
#endif