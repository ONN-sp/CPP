#ifndef LATCH_H
#define LATCH_H

#include "MutexLock.h"
#include "Condition.h"

namespace tiny_muduo
{
    class Latch{//一个同步原语,类似一个计数器   Latch就是用来确保主线程或其它线程在继续执行之前,等待新创建的线程已经准备好并且进入了可以安全等待的状态(完成初始化和准备好处理任务) 
        public:
            explicit Latch(int count) : count_(count), mutex_(), cond_(mutex_){}
            // 递减计数器值，当计数器为0时，唤醒所有等待的线程
            void CountDown(){
                MutexLockGuard lock(mutex_);
                count_--;
                if(count_ == 0)
                    cond_.NotifyAll();
            }
            
            // 等待计数器值为0
            void wait(){
                MutexLockGuard lock(mutex_);
                while(count_ > 0)
                    cond_.Wait();
            }
        private:
            int count_;
            MutexLock mutex_;
            Condition cond_;
    };
} 
#endif