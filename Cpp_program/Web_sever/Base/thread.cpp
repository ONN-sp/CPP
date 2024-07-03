#include "Thread.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <cstring>

namespace tiny_muduo{
    __thread int t_cachedTid = 0;
    __thread char t_formattedTid[32];
    __thread int t_formattedTidLength;
    int gettid(){
        return static_cast<int>(::syscall(SYS_gettid));
    }
    // 这个函数负责缓存当前线程的线程 ID
    void CacheTid(){//定义、声明都在同一命名空间中,这样才正确
        if(t_cachedTid == 0){//==0表示当前线程还未获取线程id保存到t_cachedTid中
            t_cachedTid = gettid();
            snprintf(t_formattedTid, sizeof(t_formattedTid), "%d ", t_cachedTid);
            t_formattedTidLength = std::strlen(t_formattedTid);
        }   
    }
}
using namespace tiny_muduo;

void Thread::ThreadRun() {
    // 创建一个 ThreadData 对象来包装线程的运行时数据
    ThreadData data(func_, &latch_, thread_name_);
    // 调用 RunOneFunc 来执行线程的主要功能
    data.RunOneFunc();
}

Thread::Thread(const ThreadFunc& func, const std::string& name)
        : func_(func),
          thread_name_(std::move(name)),
          latch_(1),//初始化为1,确保线程启动前同步
          started_(false)
          {}

Thread::~Thread(){
    if(started_ && thread_.joinable())
        thread_.detach();
}

void Thread::StartThread(){
    started_ = true;
    thread_ = std::thread(&Thread::ThreadRun, this);
    latch_.wait();
}

void Thread::Join(){
    if(thread_.joinable())
        thread_.join();
}