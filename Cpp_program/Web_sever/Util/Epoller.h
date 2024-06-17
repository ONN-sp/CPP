#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include "Poller.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include "noncopyable.h"

namespace tiny_muduo{
    class Channel;//前向声明  告诉编译器Channel存在,但前向声明不提供关于该类的具体信息
    class Epoller : public Poller {
    public:
        using Channels = std::vector<Channel*>;//Channel容器
        Epoller(EventLoop* loop);
        ~Epoller() override;
        //重写纯虚函数
        Timestamp Poll(Channels&) override;//实现基类Poller中纯虚函数的方法
        void UpdateChannel(Channel*) override;//实现基类Poller中纯虚函数的方法
        void RemoveChannel(Channel*) override;//从epoll中移除指定的Channel
    private:
        static const int kDefaultEvents = 16;//初始化epoll
        int EpollWait(int);//调用epoll_wait等待事件,返回事件数量
        void FillActiveChannels(int, Channels&);//将epoll_wait返回的事件填充到active_channels容器中
        void Update(int, Channel*);//更新或添加一个Channel到epoll实例中
        using Events = std::vector<struct epoll_event>;//epoll_event是一个存放epoll事件的结构体
        int epollfd_;
        Events events_;//存储epoll_wait返回的事件
        };
} 
#endif 
