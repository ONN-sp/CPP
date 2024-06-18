//定义一个独立的类来标识定时器，提高了定时器管理的安全性和效率.使得创建、管理和取消定时器的操作变得更加直观和安全，在复杂的网络应用中具有重要的意义
#ifndef TIMERID_H
#define TIMERID_H

#include <cstdint>

namespace tiny_muduo
{
class Timer;//前向声明后就不用包含头文件进来
class TimerId
{
 public:
    TimerId()
        : timer_(nullptr),
        sequence_(0)
        {}

    TimerId(Timer* timer, int64_t seq)
        : timer_(timer),
        sequence_(seq)
        {}
    friend class TimerQueue;//TimerQueue被声明为TimerID的友元类,这意味着TimerQueue可以访问TimerID类的私有成员timer_和sequence_
 private:
    Timer* timer_;
    int64_t sequence_;
};
}
#endif