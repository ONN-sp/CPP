#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include <vector>
#include <memory>
#include "../../Base/NonCopyAble.h"

namespace tiny_muduo{
    class EventLoopThread;// 前向声明
    class EventLoop;
    class EventLoopThreadPool : public NonCopyAble{
        public:
            using ThreadInitCallback = std::function<void(EventLoop*)>;
            using Thread = std::vector<std::unique_ptr<EventLoopThread>>;
            using Loop = std::vector<EventLoop*>;
            EventLoopThreadPool(EventLoop*, const std::string& name = std::string());
            ~EventLoopThreadPool();
            void SetThreadNums(int);// 设置线程数量
            void StartLoop(const ThreadInitCallback& cf = ThreadInitCallback());// 启动所有线程并开始运行它们的 EventLoop
            EventLoop* NextLoop();// 获取下一个 EventLoop，用于负载均衡
            EventLoop* GetLoopForHash(int);// 根据哈希值返回下一个EventLoop
            Loop GetAllLoops();// 获取所有的EventLoop
        private:
            EventLoop* base_loop_;// 它是整个EventLoopThreadPool的核心EventLoop对象,通常是主线程的EventLoop
            Thread threads_;// 存储所有的 EventLoopThread
            Loop loops_;// 存储所有的 EventLoop 指针,便于快速访问(不包含base_loop_)
            int thread_nums_;// 线程池中的线程数量
            int next_;// 用于循环选择下一个 EventLoop 的索引(在loops_中的索引)
            const std::string name_;// 当前EventLoopThreadPool的名称
    };
}

#endif