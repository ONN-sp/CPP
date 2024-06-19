#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <utility>
#include "../Base/Timestamp.h"
#include "../Base/NonCopyAble.h"
#include "../Base/Atomic.h"

namespace tiny_muduo{
    class Timer : public NonCopyAble{
    public:
        using BasicFunc = std::function<void()>;
        Timer(Timestamp expiration__, BasicFunc&& cb, double interval);
        // 默认析构函数
        ~Timer() = default;
        // 重启定时器,将过期时间设置为当前时间+间隔时间
        void Restart(Timestamp);
        // 运行定时器的回调函数
        void Run() const ;
        // 返回定时器的到期时间
        Timestamp expiration() const ;
        // 判断定时器是否重复运行
        bool repeat() const ;
        //获取定时器序号,用于构造定时器唯一标识符(timerId)
        int64_t sequence() const;
    private:
        Timestamp expiration_; // 定时器到期时间
        BasicFunc callback_;   // 定时器回调函数
        double interval_;      // 定时器重复间隔时间
        bool repeat_;          // 表示定时器是否重复
        const int64_t sequence_;//定时器序号
        static AtomicInt64 s_numCreated_;
    };
}
#endif
