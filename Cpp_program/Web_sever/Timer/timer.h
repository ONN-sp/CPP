#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <utility>
#include "base/timerstamp.h"

namespace tiny_muduo{
    class Timer {
    public:
        using BasicFunc = std::function<void()>;
        Timer(Timestamp expiration__, BasicFunc&& cb, double interval);
        // 默认析构函数
        ~Timer() = default;
        // 重启定时器,将过期时间设置为当前时间+间隔时间
        void Restart(Timestamp now) {
            expiration_ = Timestamp::AddTime(now, interval_);
        }
        // 运行定时器的回调函数
        void Run() const {
            if (callback_) {
            callback_();
            }
        }
        // 返回定时器的到期时间
        Timestamp expiration() const { return expiration_; }
        // 判断定时器是否重复运行
        bool repeat() const { return repeat_; }
    private:
        Timestamp expiration_; // 定时器到期时间
        BasicFunc callback_;   // 定时器回调函数
        double interval_;      // 定时器重复间隔时间
        bool repeat_;          // 表示定时器是否重复
    };
}
#endif
