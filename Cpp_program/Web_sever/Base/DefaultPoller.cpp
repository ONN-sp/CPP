#include <stdlib.h>
#include "Poller.h"
#include "Epoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop){
    if(::getenv("MUDUO_USE_POLL"))
        return nullptr;//本项目没有muduo中的pollpoller
    else
        return new Epoller(loop);
}