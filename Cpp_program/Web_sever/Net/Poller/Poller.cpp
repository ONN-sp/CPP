#include "Poller.h"
#include "../Util/Channel.h"
#include "../Util/EventLoop.h"
#include "../../Base/Timestamp.h"

using namespace tiny_muduo;

Poller::Poller(EventLoop* loop) : ownerLoop_(loop){}

bool Poller::hasChannel(Channel* channel) const{
    auto it = channels_.find(channel->fd());//找不到的话it等于迭代器的end
    return it!= channels_.end() && it->second == channel;
}

