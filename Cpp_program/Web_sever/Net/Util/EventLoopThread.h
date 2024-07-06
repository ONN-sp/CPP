#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include "../../Base/NonCopyAble.h"
#include "../../Base/Condition.h"
#include "../../Base/MutexLock.h"
#include "../../Base/Thread.h"
#include <functional>

namespace tiny_muduo{
    class EventLoop;
    class EventLoopThread : public NonCopyAble {
        public:
            using ThreadInitCallback = std::function<void(EventLoop*)>;
            EventLoopThread(const ThreadInitCallback& cf = ThreadInitCallback(),
                  const std::string& name = std::string());// 提供了默认值,这样在调用EventLoopThread构造函数时,可以省略参数.即ThreadInitCallback为空回调
            ~EventLoopThread();
            void StartFunc();// 线程的启动函数
            EventLoop* StartLoop();// 启动 EventLoop 并返回指向 EventLoop 的指针
        private:
            std::unique_ptr<EventLoop> loop_;
            Thread thread_;
            MutexLock mutex_;
            Condition cond_;
            const std::string name_;
            bool exiting_;  // 标识线程是否已退出
            ThreadInitCallback callback_;// 一个用于线程初始化的回调函数(通常不存在,除非自己指定)
    };
}

#endif