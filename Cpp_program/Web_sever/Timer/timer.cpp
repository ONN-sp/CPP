#include "Timer.h"
#include <utility>
#include "TimerId.h"

using namespace tiny_muduo;

Timer::Timer(Timestamp expiration__, BasicFunc&& cb, double interval = 0.0)//下面这些初始化操作不涉及拷贝
            : expiration_(expiration__),// 设置定时器的过期时间
              callback_(std::move(cb)),// 设置定时器的回调函数  // 构造函数使用了 C++11 的默认参数和右值引用 (&&)
              interval_(interval),//设置定时器的时间间隔
              repeat_(interval > 0.0),//如果interval大于0,则定时器会重复运行
              sequence_(s_numCreated_.incrementAndGet())//获取定时器序列
              {}

// 重启定时器,将过期时间设置为当前时间+间隔时间
void Timer::Restart(Timestamp now) {
    expiration_ = Timestamp::AddTime(now, interval_);
}

// 运行定时器的回调函数
void Timer::Run() const {
    if (callback_) {
    callback_();
    }
}

// 返回定时器的到期时间
Timestamp Timer::expiration() const {return expiration_;}
// 判断定时器是否重复运行
bool Timer::repeat() const {return repeat_;}
//获取定时器序号,用于构造定时器唯一标识符(timerId)
int64_t Timer::sequence() const {return sequence_;}