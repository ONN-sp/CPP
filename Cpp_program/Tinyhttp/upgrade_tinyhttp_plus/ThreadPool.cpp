#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>
#include <functional>
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t thread_nums):stop(false){//构造函数:向线程池添加线程(线程里面要利用条件变量控制实现任务执行).在这个构造函数中并没有直接调用std::thread的构造函数来创建线程,而是通过emplace_back()函数以lambda函数作为参数来实现线程的创建
    for(int i=0;i<thread_nums;i++){
        threads.emplace_back([this](){//threads是一个元素为std::thread的数组,因此这个lambda函数进来会调用thread的构造函数默认构造一个thread变量,而这个thread线程的入口函数就是这个lambda表达式
            while(1){//线程不能终止,要一直执行任务队列的任务
                std::function<void()> task;//因为后续用的是std::bind(),所以这个函数包装器中的参数可以为空
                {
                std::unique_lock<std::mutex> lock(this->task_mtx);
                this->cond.wait(lock, [this](){return !this->tasks.empty() || this->stop;});
                if(this->stop || this->tasks.empty())
                    return;
                task = std::move(this->tasks.front());
                this->tasks.pop();
                }    
                task();//task任务的执行不能被锁住,如果锁住那么就可能会导致某个线程一直执行任务,而其它线程被锁住
            }
        });
    }
}

ThreadPool::~ThreadPool(){//析构函数中会等作用域结束自动调用
    {
        std::unique_lock<std::mutex> lock(this->stop_mtx);
        this->stop = true;//代表这个线程池结束了  
    }
    cond.notify_all();
    for(auto& t:threads)
        t.join();
}




   
   