#include <stdlib.h>
#include "../Util/Poller.h"
#include "../Util/Epoller.h"

using namespace tiny_muduo;

Poller* Poller::newDefaultPoller(EventLoop* loop){
    if(::getenv("MUDUO_USE_POLL"))
        return nullptr;//本项目没有muduo中的pollpoller
    else
        return new Epoller(loop);
}