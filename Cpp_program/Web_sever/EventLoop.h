#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>
#include "base\noncopyable.h"
#include <functional>
#include <memory>
#include "timerstamp.h"
#include "timerQueue.h"
#include "mutex.h"
#include "channel.h"
#include "Epoller.h"

namespace tiny_muduo{
class Epoller;
class Channel;
class EventLoop : public NonCopyAble{
    public:
        using BasicFunc = std::function<void()>;
        using Channels = std::vector<std::shared_ptr<Channel>>;
        using ToDoList = std::vector<BasicFunc>;

        EventLoop();
        ~EventLoop();

        void RunAt(Timestamp, BasicFunc&&);//向定时器队列添加定时器任务
        void RunAfter(double, BasicFunc&&);//在指定事件后允许一次BasicFunc任务
        void RunEvery(double, BasicFunc&&);//每隔指定时间运行一次任务
        bool IsInThreadLoop();//判断当前线程是否在事件循环线程中运行

        void wakeup();//通过eventfd唤醒loop所在的线程    
        //EventLoop方法=>Poller的方法
        void UpdateChannel(Channel*);
        void RemoveChannel(Channel*);
        bool hasChannel(Channel* channel);

        void loop();//开启事件循环
        void QueueOneFunc(BasicFunc);//将一个任务添加到任务队列中
        void RunOneFunc(BasicFunc);//执行一个任务
        void quit();//退出事件循环
    private:
        void handleRead();//给eventfd返回的文件描述符wakeup_fd_绑定的事件进行回调
        void doPendingFunctors(); //执行上层回调
        bool running_; // 标志事件循环是否在运行
        bool quit_;// 标志事件循环是否退出 
        pid_t tid_; // 记录当前EventLoop是被哪个线程id创建的  即标识了当前EventLoop的所属线程id,这个是标识当前EventLoop所属的线程id
        std::unique_ptr<Epoller> epoller_; // 管理事件的 Epoller 对象
        int wakeup_fd_; // 事件循环使用的唤醒文件描述符  当主mainloop获取一个新用户的Channel(即获得一个文件描述符)  需要轮询算法选择一个subloop  通过wakeup_fd_唤醒subloop
        std::unique_ptr<Channel> wakeup_channel_; // 唤醒事件循环的 Channel 对象
        std::unique_ptr<TimerQueue> timer_queue_; // 定时器队列管理对象
        bool calling_functors_; // 标志当前loop是否有需要执行的回调函数
        Channels active_channels_; // 活跃的 Channel 列表
        ToDoList pendingFunctors_; // 存储loop需要执行的所有回调操作(上层给定需要执行回调函数向量)  这个向量
        MutexLock mutex_; // 互斥锁，保护共享资源
        const int KPollTimeMs = 10000;// Poll方法调用的超时时间
};
}
#endif