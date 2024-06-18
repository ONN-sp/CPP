//基类
#ifndef POLL_H
#define POLL_H
#include <vector>
#include <unordered_map>
#include "Timerstamp.h"
#include "NonCopyAble.h"

namespace tiny_muduo {
    class Channel;//前向声明
    class EventLoop;
    class Poller : public NonCopyAble {
    public:
        using Channels = std::vector<Channel*>;//Channel容器
        Poller(EventLoop*);
        virtual ~Poller() = default;
        //给所有I/O复用(因为可能用select poll epoll)保留统一的接口
        virtual Timestamp Poll(int , Channels&) = 0;//在不超时下调用epoll_wait等待事件,并填充就绪的Channel到channels容器
        virtual void UpdateChannel(int , Channel*) = 0;//修改或添加一个Channel到epoll实例中
        virtual void RemoveChannel(Channel*) = 0;
        bool hasChannel(Channel*) const;//判断当前channel是否在当前的poller中
        static Poller* newDefaultPoller(EventLoop*);//EventLoop可以通过调用该接口函数获取默认的I/O复用的具体实现
    protected:
        using ChannelMap = std::unordered_map<int, Channel*>;//哈希表  文件描述符->Channel的映射
        ChannelMap channels_;
    private:
        EventLoop* ownerLoop_;//定义Poller所属的事件循环
    };
} 
#endif 
