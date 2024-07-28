#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H

#include <unistd.h>
#include <sys/timerfd.h>
#include <set>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <cstdint>   // for uint64_t
#include "Timer.h"     
#include "../../Base/Timestamp.h" 
#include "../../Base/NonCopyAble.h" 
namespace tiny_muduo{
    class EventLoop;
    class Channel;
    class TimerId;
    //TimerQueue类管理多个定时器,并在定时器到期时触发回调
    class TimerQueue : public NonCopyAble{
        public:
            using BasicFunc = std::function<void()>;
            TimerQueue(EventLoop* loop);
            ~TimerQueue();
            //读取timerfd的内容,用于更新定时器状态
            void ReadTimerfd();
            //处理定时器事件
            void HandleRead();
            //重置所有需要重复的定时器,并将它们重新插入到定时器集合中
            void ResetTimers();
            //向定时器集合中插入一个新的定时器
            bool Insert(Timer*);          
            //在事件循环中添加一个定时器
            void AddTimerInLoop(Timer*);         
            //重置定时器的fd
            void ResetTimer(Timer*);
            TimerId AddTimer(Timestamp, BasicFunc&&, double);
        private:
            // 定义一个pair类型，用于存储定时器的过期时间和指向定时器的指针
            using TimerPair = std::pair<Timestamp, Timer*>;//为了处理两个到期时间相同的情况,这里使用std::pair<Timestamp, Timer*>作为std::set的key,这样就可以区分到期时间形相同的定时器
            // 定义一个set类型，用于按过期时间排序存储定时器
            using TimersSet = std::set<TimerPair>;  
            // 定义一个vector类型，用于存储活动的定时器
            using ActiveTimers = std::vector<TimerPair>;
            EventLoop* loop_; // 指向事件循环的指针
            int timerfd_;     // timerfd的文件描述符
            std::unique_ptr<Channel> channel_; // 管理定时器事件的通道
            TimersSet timers_; // 定时器集合，按过期时间排序
            ActiveTimers active_timers_; // 活动的定时器集合,即定时完成的定时器(过期定时器),然后可以执行回调函数了(即"活了")
    };
}
#endif