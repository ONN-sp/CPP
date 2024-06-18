//Channel处于底层   具体的回调函数在上层  Channel和文件描述符绑定在一起
#ifndef CHANNEL_H
#define CHANNEL_H

#include "NonCopyAble.h"
#include <functional>
#include "callback.h"

//Channel理解为通道,封装了sockfd和其感兴趣的事件  如EPOLLIN(读)、EPOLLOUT(写)事件  还绑定了poller返回的具体事件
namespace tiny_muduo{
    // 使用enum class来定义ChannelState 
    enum class ChannelState{
        kNew, //新的Channel     Channel 刚创建,尚未加入到 epoll 实例中
        kAdded,//Channel已添加  Channel 已经被添加到 epoll 实例中,并正在监控其感兴趣的事件
        kDeleted//Channel已删除  Channel 已经从 epoll 实例的监控列表(即epoll树)中删除了
    };
    class Channel : public NonCopyAble{
        public:
        Channel(EventLoop* loop, int fd);
        ~Channel();
        //根据recv_events_的值分别调用不同的处理事件
        void HandleEvent();
        //带有保护的处理事件
        void HandleEventWithGuard();
        //设置读回调函数(移动语义)
        void SetReadCallback(ReadCallback&&);
        //设置读回调函数(拷贝语义  参数传递)
        void SetReadCallback(const ReadCallback&);
        //设置写回调函数(移动语义)
        void SetWriteCallback(WriteCallback&&);
        //设置写回调函数(拷贝语义)
        void SetWriteCallback(const WriteCallback&);
        //设置错误回调函数(移动语义)
        void SetErrorCallback(ErrorCallback&&);
        //设置错误回调函数(移动语义)
        void SetErrorCallback(const ErrorCallback&);
        //将Channel绑定到某个对象的生命周期上,避免使用时对象已被销毁
        void Tie(const std::shared_ptr<void>&);
        //启动读事件
        void EnableReading();
        //启用写事件
        void EnableWriting();
        //禁用所有事件
        void DisableAll();
        //禁用写事件
        void DisableWriting();
        //更新事件状态
        void Update();
        //设置接收到的事件
        void SetReceiveEvents(int);
        //设置Channel的状态
        void SetChannelState(ChannelState);
        //获取文件描述符
        int fd() const;
        //获取事件标志
        int events() const;
        //获取接收到的事件标志
        int recv_events() const;
        //获取Channel的状态
        ChannelState state() const;
        //判断是否在监听写事件
        bool IsWriting() const;
        //判断是否在监听读事件
        bool IsReading() const;
    private:
        EventLoop* loop_; // 关联的事件循环  一个eventloop有多个fd,则它有多个channel   两个channel不能在同一个eventloop中
        int fd_; // 文件描述符
        int events_; // 感兴趣的事件数   将感兴趣的事件注册到epoll中  由用户设置
        int recv_events_; // epoller返回的事件数 目前活动(就绪)的事件   由epoller/EventLoop设置  
        std::weak_ptr<void> tie_; // 用于绑定的对象
        bool tied_; // 是否绑定了对象
        int errno_; // 错误号(通常在事件处理过程中设置)

        ChannelState state_; // Channel 的状态
        ReadCallback read_callback_; // 读事件的回调函数
        WriteCallback write_callback_; // 写事件的回调函数
        ErrorCallback error_callback_; // 错误事件的回调函数
    }
}

#endif