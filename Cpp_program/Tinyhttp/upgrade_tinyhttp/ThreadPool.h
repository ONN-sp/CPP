#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>
#include <functional>

#ifndef THREADPOOL_H
#define THREADPOOL_H

class ThreadPool{
    private:
        std::vector<std::thread> threads;
        std::queue<std::function<void()>> tasks;
        std::condition_variable cond;
        std::mutex stop_mtx;
        std::mutex task_mtx;
        bool stop;
    public:
        ThreadPool(size_t thread_nums);
        ~ThreadPool();
        template <class F, class... Args>
        void enqueue(F &&f, Args&&... arg){//任务队列函数(向队列中加任务).函数模板里面右值引用=万能引用
            {
                std::unique_lock<std::mutex> lock(task_mtx);
                tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(arg)...));//std::bind将函数和函数参数绑定,std::forward表示完美转化(也转化为万能引用)
            }
            cond.notify_one();//任务队列中加了任务,通知线程数组中的线程去执行
        }
};
#endif

   
   