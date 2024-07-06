#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

using namespace tiny_muduo;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* loop, const std::string& name)
    : base_loop_(loop),// 通常是主线程中的 EventLoop
      thread_nums_(0),
      name_(name),
      next_(0)// 初始下一个 EventLoop 的索引为 0
      {}

EventLoopThreadPool::~EventLoopThreadPool(){}// 由于threads_使用了std::unique_ptr,所以析构函数会自动清理动态分配的EventLoopThread对象
// 设置线程数量
void EventLoopThreadPool::SetThreadNums(int thread_nums) {
     thread_nums_ = thread_nums;
}
// 启动线程池并创建指定数量的 EventLoop 线程
void EventLoopThreadPool::StartLoop(const ThreadInitCallback& cf){
    for(int i=0;i<thread_nums_;i++){
        auto thread = std::make_unique<EventLoopThread>(cf, name_);
        threads_.emplace_back(std::move(thread));
        loops_.emplace_back(thread->StartLoop());
    }
    if(thread_nums_==0&&cf)// 如果线程数为0(表示只有主线程baseLoop_,这样的原因是避免cf被错误地执行多次)且有初始化回调函数,则在base_loop_上执行回调函数
        cf(base_loop_);
}
// 获取下一个EventLoop对象,用于负载均衡  这里是直接线性(++)找下一个EventLoop
EventLoop* EventLoopThreadPool::NextLoop(){
    EventLoop* ret = base_loop_;
    if(!loops_empty()){
        ret = loops_[next_++];// 返回下一个EventLoop
        if(next_ >= loops_.size())
            next_ = 0;// 循环使用loops中的EventLoop
    }
    return ret;
}
// 根据哈希值获取下一个EventLoop对象
EventLoop* EventLoopThreadPool::GetLoopForHash(int hashCode){
    EventLoop* ret = base_loop_;
    if(!loops_empty())
        ret = loops_[hashCode%loop_.size()];// 根据哈希值返回下一个EventLoop
    return ret;
}
// 获取所有的 EventLoop
Loop EventLoopThreadPool::GetAllLoops(){
    if(loops_.empty())
        return std::vector<EventLoop*>(1, base_loop_);// vector的构造函数初始化要指定元素个数    此时只返回主线程的base_loop_
    else if
        return loops_;
}
