#include "EventLoop.h"
#include "CurrentThread.h"
#include "../../Base/Timestamp.h"
#include "../Timer/TimerQueue.h"
#include <cassert>
#include <signal.h>
#include <sys/eventfd.h>
#include <iostream>
#include "../Poller/Epoller.h"

using namespace tiny_muduo;

namespace {
    //在服务器端向已经关闭的客户端socket写数据时,如果不处理SIGPIPE信号,服务器可能会因为信号导致异常终止
    //通常情况下,服务器希望继续运行而不因为客户端关闭连接而终止,因此需要忽略或者处理 SIGPIPE 信号
    class IgnoreSigPipe{// 定义一个类 IgnoreSigPipe,用于忽略 SIGPIPE 信号
        public:
            IgnoreSigPipe(){
                ::signal(SIGPIPE, SIG_IGN);//设置SIGPIPE信号的处理函数为忽略
            }
    };
    IgnoreSigPipe initObj;// 创建一个静态对象 initObj，用于在程序启动时初始化 IgnoreSigPipe 类，以达到忽略 SIGPIPE 信号的效果
}

EventLoop::EventLoop()
        : running_(false),
          quit_(false),
          tid_(CurrentThread::tid()),//获取当前线程ID
          epoller_(std::make_unique<Epoller>(this)),//创建Epoller对象,用于I/O复用
          wakeup_fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),//创建 eventfd,用于线程间的唤醒
          wakeup_channel_(std::make_unique<Channel>(this, wakeup_fd_)),//创建wakeup_fd_对应的Channel对象   
          timer_queue_(std::make_unique<TimerQueue>(this)),// 创建TimerQueue,用于管理定时器
          calling_functors_(false)//初始化当前loop没有需要执行的回调函数
          {
            //设置唤醒Channel的读回调函数为HandleRead
            wakeup_channel_->SetReadCallback([this]{this->HandleRead();});//wakeup是用wirte实现唤醒的,所以是读回调函数
            //启用唤醒Channel的读事件监听
            wakeup_channel_->EnableReading();
          }

EventLoop::~EventLoop() {
    if (running_) running_ = false; // 停止事件循环
    wakeup_channel_->DisableAll(); // 禁用唤醒 Channel 的所有事件监听
    RemoveChannel(wakeup_channel_.get()); // 从 Epoller 中移除唤醒 Channel  智能指针的.get()方法
    ::close(wakeup_fd_); // 关闭唤醒的文件描述符
}

bool EventLoop::IsInThreadLoop(){//判断当前EventLoop对象是否在自己的线程里   通过判断当前EventLoop所属的id与调用这个EventLoop的线程的线程id是不是相同的来进行识别
    return CurrentThread::tid() == tid_; 
}

void EventLoop::loop() {
    assert(IsInThreadLoop()); // 确保当前线程是事件循环所在的线程
    running_ = true; // 标志事件循环开始运行
    quit_ = false;// 是否退出loop
    while (!quit_) {
        active_channels_.clear(); // 清空活跃 Channel 列表
        epoller_->Poll(KPollTimeMs, active_channels_); // 调用 Epoller 的 Poll 函数获取活跃的 Channel 列表
        for (const auto& channel : active_channels_)
            channel->HandleEvent(); // Poller监听哪些Channel发生了事件,然后上报给EventLoop,EventLoop再通知channel处理相应的事件
        doPendingFunctors(); // 处理待回调函数列表中的任务 
    }
    running_ = false; // 事件循环结束
}

void EventLoop::quit(){
    quit_ = true;
    if(!IsInThreadLoop())
        wakeup();//如果此时这个子EventLoop被阻塞了,那么退出事件需要先唤醒
}

//wakeup_fd_对应的读回调函数
void EventLoop::HandleRead() {
    uint64_t read_one_byte = 1;
    ssize_t read_size = ::read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));
    (void) read_size; // 防止编译器警告未使用的变量
    assert(read_size == sizeof(read_one_byte)); // 确保读取的字节数正确
    return;
}

void EventLoop::QueueOneFunc(BasicFunc func) {
    {
        MutexLockGuard lock(mutex_); // 加锁保护待执行任务列表
        pendingFunctors_.emplace_back(std::move(func)); // 将回调任务添加到待执行任务列表末尾
    }

    // 如果不在事件循环线程中,或者正在执行任务函数,需要唤醒事件循环
    if (!IsInThreadLoop() || calling_functors_)//新加进来的任务不是已经被执行了的,即此时加进来的是下一轮要去执行的,那么需要`wakeup`
        wakeup();
}

void EventLoop::RunOneFunc(BasicFunc func) {
    if (IsInThreadLoop())
        func(); // 如果当前调用RunOneFunc的线程的线程id=RunOneFunc函数所属的EventLoop所属的线程id,则直接执行任务函数
    else
        QueueOneFunc(std::move(func)); // 在非当前EventLoop所属线程的线程中调用RunOneFunc,就将对应的任务添加到待执行任务列表中,并且要唤醒EventLoop所属的线程执行QueueOneFunc
}

void EventLoop::doPendingFunctors() {
    ToDoList functors;// 局部变量
    calling_functors_ = true; // 标志开始执行任务函数
    {
        MutexLockGuard lock(mutex_); // 加锁保护待执行任务列表
        functors.swap(pendingFunctors_); // 将待执行任务列表交换到局部变量中  这种交换方式减少了锁的临界区范围(这种方式把pendingFunctors_放在了当前线程的栈空间中,此时这个Functors局部变量就线程安全了),提升了效率,同时避免了死锁(因为Functor可能再调用queueInLoop)
    }
    // 依次执行待执行任务列表中的任务函数
    for (const auto& func : functors)
        func();
    calling_functors_ = false; // 标志任务函数执行结束
}

void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_fd_, &one, sizeof(one));//唤醒是用wirte实现的,这样就不会让epoll_wait阻塞了
    if(n!= sizeof(one))
        std::cout << "wakeup error" << std::endl;//TODO:modify?
}

void EventLoop::UpdateChannel(Channel* channel){
    epoller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel){
    epoller_->RemoveChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
    return epoller_->hasChannel(channel);
}
// 将指定的定时器(包括对应的定时回调函数)加入当前EventLoop(即当到了指定的时间timestamp时,就会调用相应的回调函数cb)  并且只执行一次interval=0.0
void EventLoop::RunAt(Timestamp timestamp, BasicFunc&& cb) {
    timer_queue_->AddTimer(timestamp, std::move(cb), 0.0);
}
// 将在当前时间后指定等待时间的一个定时器(包括对应的定时回调函数)加入当前EventLoop  并且只执行一次interval=0.0
void EventLoop::RunAfter(double wait_time, BasicFunc&& cb) {
    Timestamp timestamp(Timestamp::AddTime(Timestamp::Now(), wait_time)); 
    timer_queue_->AddTimer(timestamp, std::move(cb), 0.0);
}
// 将在当前时间后指定等待时间的一个定时器(包括对应的定时回调函数)加入当前EventLoop  并且每隔interval时间执行一次
void EventLoop::RunEvery(double interval, BasicFunc&& cb) {
    Timestamp timestamp(Timestamp::AddTime(Timestamp::Now(), interval)); 
    timer_queue_->AddTimer(timestamp, std::move(cb), interval);
}