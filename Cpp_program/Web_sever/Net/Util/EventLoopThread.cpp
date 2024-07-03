//IO线程不一定是主线程,我们可以在任何一个线程创建并允许EventLoop.EventLoopThread类正和one loop per thread对应
#include "EventLoopThread.h"
#include "EventLoop.h"

using namespace tiny_muduo;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cf,
                                 const std::string& name)
    : loop_(nullptr),
      exiting_(false),  // 初始化退出标志为 false
      thread_(std::bind(&EventLoopThread::StartFunc, this), name),
      mutex_(),
      cond_(mutex_),
      callback_(cf)// 初始化线程启动时的回调函数
      {}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;// 退出
    if(loop_!=nullptr){
        loop_->quit();// 通知 EventLoop 退出
        thread_.Join();// 等待线程结束
    }
}
// 启动 EventLoop 并返回指向 EventLoop 的指针
EventLoop* EventLoopThread::StartLoop(){
    thread_.StartThread();// 启动线程
    EventLoop* loop = nullptr;
    {
        MutexLockGuard lock(mutex_);
        while(loop_==nullptr)// 不能是if !!!
            cond_.wait();
        loop = loop_;// 获取初始化后的 loop_
    }
    return loop;
}
// 线程函数，创建并运行 EventLoop
void EventLoopThread::StartFunc(){
    EventLoop loop;// 一个loop和一个线程绑定(thread_)
    if(callback_)// 如果存在启动EventLoopThread伴随的回调函数就先回调(但是一般是没有的,只是一个空回调)
        callback_(&loop);
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.Notify();
    }
    loop_->loop();// 运行 EventLoop 的主循环
    {
        MutexLockGuard lock(mutex_);
        loop_ = nullptr;
    }
}

