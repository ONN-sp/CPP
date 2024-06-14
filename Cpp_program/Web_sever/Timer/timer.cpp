#include "timer.h"
#include <utility>

using namespace tiny_muduo;

Timer::Timer(Timestamp expiration__, BasicFunc&& cb, double interval = 0.0)
            : expiration_(expiration__),// 设置定时器的过期时间
              callback_(std::move(cb)),// 设置定时器的回调函数  // 构造函数使用了 C++11 的默认参数和右值引用 (&&)
              interval_(interval),//设置定时器的时间间隔
              repeat_(interval > 0.0) {//如果interval大于0,则定时器会重复运行
        }
