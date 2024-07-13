#include "TimerQueue.h"
#include "../Util/Channel.h"
#include "../Util/EventLoop.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include "../../Base/Logging/Logging.h"

using namespace tiny_muduo;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
      channel_(std::make_unique<Channel>(loop_, timerfd_)){// 使用std::make_unique创建Channel对象
  channel_->SetReadCallback([this]{this->HandleRead();});// 设置channel的读回调函数，一旦有数据可读就调用HandleRead处理
  channel_->EnableReading();// 启用channel的读事件监听
}

TimerQueue::~TimerQueue() {
  channel_->DisableAll();// 禁用所有channel事件
  loop_->RemoveChannel(channel_.get());// 从事件循环中移除channel
  ::close(timerfd_);// 使用作用域解析运算符表示调用的是全局命名空间中的close,而非当前作用域内的其它可能存在的同名函数
  for (const auto& timerpair:timers_){//释放定时器资源
    delete timerpair.second;//没用智能指针,所以必须手动释放
  }
}
// 添加定时器(包括定时时间,定时完成的回调函数)到定时器队列中
TimerId TimerQueue::AddTimer(Timestamp timestamp, BasicFunc&& cb, double interval) {
  Timer* timer = new Timer(timestamp, std::move(cb), interval);//此时需要手动delete(delete timerpair.second;)   TODO:也许可以用智能指针std::unique_ptr<Timer>
  loop_->RunOneFunc([this, timer]{this->AddTimerInLoop(timer);});// 在事件循环中异步执行AddTimerInLoop函数，将定时器加入到定时器队列中
  return TimerId(timer, timer->sequence());//返回新加入的定时器的唯一标识符timerId
}

void TimerQueue::ResetTimer(Timer* timer) {
  struct itimerspec new_value;
  struct itimerspec old_value;
  std::memset(&new_value, 0, sizeof(new_value));// 清零新的timerfd设置
  std::memset(&old_value, 0, sizeof(old_value));// 清零旧的timerfd设置
  int64_t microseconds_diff = timer->expiration().microseconds() - Timestamp::Now().microseconds();// 计算定时器到期时间与当前时间的微秒差值
  if (microseconds_diff < 100){// 如果微秒差值小于100，设定为100微秒，避免立即过期
    microseconds_diff = 100;
  }
  new_value.it_value.tv_sec = static_cast<time_t>(microseconds_diff / 1000000);// 设置新的timerfd定时器值
  new_value.it_value.tv_nsec = static_cast<long>((microseconds_diff % 1000000) * 1000);
  int ret = ::timerfd_settime(timerfd_, 0, &new_value, &old_value);// 调用timerfd_settime设置新的timerfd定时器
  assert(ret != -1);// 确保设置定时器成功
  (void) ret;// 防止编译器警告未使用的ret变量
}

//重置所有需要重复的定时器,并将它们重新插入到定时器集合中
void TimerQueue::ResetTimers(){
    for(auto& timerpair:active_timers_){
        if((timerpair.second)->repeat()){
            (timerpair.second)->Restart(Timestamp::Now());
            Insert(timerpair.second);
        }
        else
            delete timerpair.second;
    }
    //如果还有其它定时器,重置最近的一个定时器
    if(!timers_.empty()){
        ResetTimer(timers_.begin()->second);
    }
}

//向定时器集合中插入一个新的定时器
bool TimerQueue::Insert(Timer* timer){
    bool reset_instantly = false;
    if(timers_.empty()||timer->expiration()<timers_.begin()->first)
        reset_instantly = true;
    timers_.emplace(timer->expiration(), timer);
    return reset_instantly;
}

//在事件循环中添加一个定时器
void TimerQueue::AddTimerInLoop(Timer* timer){
    bool reset_instantly = Insert(timer);
    if (reset_instantly) {
        ResetTimer(timer);
}
}

 //处理定时器事件
void TimerQueue::HandleRead(){
    ReadTimerfd();
    Timestamp now = Timestamp::Now();
    active_timers_.clear(); // 清空当前活动的定时器集合,以便装入新的到期定时器(到期定时器<=>可以活动的定时器)
    //找到所有到期的定时器
    auto end = timers_.lower_bound(TimerPair(now, nullptr));// 查找timers_中第一个不小于当前时间的定时器  "<"运算符在timestamp中重载了的
    active_timers_.insert(active_timers_.end(), timers_.begin(), end);// 在active_timers_.end()定时器之前插入timers_.begin()到end的所有定时器元素(timers_中是有序的)   将所有到期的定时器(从集合开始到end迭代器之间的定时器)插入到active_timers_中
    timers_.erase(timers_.begin(), end); // 从timers_集合中移除这些到期的定时器,避免重复处理
    //运行所有到期的定时器的回调函数  定时完成<=>到期,即调用回调函数
    for(const auto&timerpair:active_timers_)
        timerpair.second->Run();
    // 重置所有需要重复的定时器
    ResetTimers();// 重新设置定时器,处理那些需要周期性重复触发的定时器
}

//读取timerfd的内容,用于更新定时器状态
void TimerQueue::ReadTimerfd(){
    uint64_t read_byte;
    ssize_t readn = ::read(timerfd_, &read_byte, sizeof(read_byte));
    if(readn!=sizeof(read_byte))
        LOG_ERROR << "TimerQueue::ReadTimerFd read_size < 0";
}