#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "Condition.h"
#include "MutexLock.h"
#include "Thread.h"
#include <vector>
#include <memory>
#include <functional>
#include <deque>

namespace tiny_muduo{
    class ThreadPool : public NonCopyAble{
        public:
            using Task = std::function<void()>;
            explicit ThreadPool(const std::string& nameArg = "ThreadPool");
            ~ThreadPool();
            void setMaxQueueSize(int);// 设置任务队列的最大长度
            void setThreadInitCallback(const Task&)// 设置线程初始化回调函数
            void start(int);// 启动线程池，创建指定数量的线程
            void stop();// 停止线程池，等待所有线程结束
            const std::string& name() const{// 返回线程池的名称
                return name_;
            }
            int queueSize() const;// 返回当前任务队列的大小(任务数)
            void run(Task); // 向线程池提交任务
        private:
            bool isFull() const;// 判断任务队列是否已满
            void runInThread();// 线程执行的主函数,从任务队列中取出任务执行
            Task take();// 从任务队列中取出一个任务  这里只取任务,不执行

            mutable MutexLock mutex_;// mutable表示允许在const成员函数中修改mutex_成员变量
            Condition notEmpty_ GUARDED_BY(mutex_);// 表示非空的条件变量,表示任务队列非空     表示条件变量notEmpty_是在mutex_互斥锁保护下使用的,即这个条件变量的wait  notify操作应该在持有mutex_的锁的情况下进行
            Condition notFull_ GUARDED_BY(mutex_);// 非满条件变量，表示任务队列未满
            std::string name_;// 线程池的名称
            Task threadInitCallback_;// 线程初始化回调函数
            std::vector<std::unique_ptr<Thread>> threads_;// 线程对象数组
            std::deque<Task> queue_ GUARDED_BY(mutex_);// 任务队列(双端队列)  表示任务队列queue_是在mutex_互斥锁保护下使用的,即它的pop  push等操作应该在持有mutex_这把锁的情况下才能进行
            int maxQueueSize_;// 任务队列最大长度
            bool running_;  // 线程池运行状态标志
    };
}
#endif