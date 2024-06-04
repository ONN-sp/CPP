//简单线程池+动态管理线程数+任务优先级调用
#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>
#include <future>
#include <algorithm>
#include <functional>

namespace ljj{
    class Compare{
        public:
        bool operator()(const std::pair<int, std::function<void()>> ls, const std::pair<int, std::function<void()>> rs){
            return ls.first > rs.first;//小顶堆,数值越小优先级越高
        }
    };
    class ThreadPool{
        private:
            std::vector<std::thread> threads;
            std::priority_queue<std::pair<int, std::function<void()>>, 
                                std::vector<std::pair<int, std::function<void()>>>, 
                                Compare> tasks; //使用自定义的比较器 
            std::condition_variable cond;
            std::mutex stop_mtx;
            bool stop;
            size_t thread_nums;
            size_t min_threads;
            size_t max_threads;
        public:
            ThreadPool(size_t min_thread_nums, size_t max_thread_nums):min_threads(min_thread_nums), max_threads(max_thread_nums), thread_nums(min_thread_nums), stop(false){
                for(int i=0; i<thread_nums; i++){
                    threads.emplace_back([this](){
                        while (true){
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(this->stop_mtx);
                                this->cond.wait(lock, [this]() {return this->stop || !this->tasks.empty();});
                                if (this->stop && this->tasks.empty())
                                    return;
                                task = std::move(this->tasks.top().second);
                                this->tasks.pop();
                            }
                            task();
                        }
                    });
                }
            }

            ~ThreadPool() {
                {
                    std::unique_lock<std::mutex> lock(stop_mtx);
                    stop = true;
                }
                cond.notify_all();
                for (auto& t : threads)
                    t.join();
            }

            template<class F, class... Args>
            void enqueue(int priority, F &&f, Args &&... arg) {//主线程
                {
                    std::unique_lock<std::mutex> lock(stop_mtx);
                    tasks.emplace(priority, std::bind(std::forward<F>(f), std::forward<Args>(arg)...));
                }  
                cond.notify_one();         
            }

            auto set_newthread_nums(size_t newthread_nums){//主线程
                if(newthread_nums<min_threads)
                    newthread_nums=min_threads;
                else if(newthread_nums > max_threads)
                    newthread_nums=max_threads;
                if(newthread_nums>thread_nums){
                    for(int i=0; i<newthread_nums-thread_nums; i++){
                        threads.emplace_back([this]() {
                            while (true) {
                                std::function<void()> task;
                                {
                                    std::unique_lock<std::mutex> lock(this->stop_mtx);
                                    this->cond.wait(lock, [this]() { return this->stop || !this->tasks.empty(); });
                                    if (this->stop && this->tasks.empty())
                                        return;
                                    task = std::move(this->tasks.top().second);
                                    this->tasks.pop();
                                }
                                task();
                            }
                        });
                    }
                }
                else if(newthread_nums<thread_nums){//思路:先删除所有线程,再重新创建newthread_nums个线程
                    {
                        std::unique_lock<std::mutex> lock(stop_mtx);
                        stop = true;//删除标志
                    }
                    cond.notify_all();
                    for (std::thread& worker : threads)
                        worker.join();
                    threads.erase(threads.begin(), threads.end());
                    {
                        std::unique_lock<std::mutex> lock(stop_mtx);
                        stop = false;
                    }
                    for(int i=0; i<newthread_nums; i++){
                        threads.emplace_back([this]() {
                            while (true) {
                                std::function<void()> task;
                                {
                                    std::unique_lock<std::mutex> lock(this->stop_mtx);
                                    this->cond.wait(lock, [this]() { return this->stop || !this->tasks.empty(); });
                                    if (this->stop && this->tasks.empty())
                                        return;
                                    task = std::move(this->tasks.top().second);
                                    this->tasks.pop();
                                }
                                task();
                            }
                        });
                    }
                }
                thread_nums=threads.size();
                return thread_nums;
            }
    };
};

int main() {
    ljj::ThreadPool pool(2, 10); //初始化的最小线程数=2,最大线程数=10
    size_t tasks_size = 6;
    for (int i = 0; i < 6; ++i) {
        pool.enqueue(i, [i] {
            std::cout << "Task " << i << " executed\n";  
            std::this_thread::sleep_for(std::chrono::seconds(1));    
        });
    }
    std::this_thread::sleep_for(std::chrono::seconds(5)); 
    size_t threads_nums = pool.set_newthread_nums(tasks_size);//执行一部分任务自动调整线程池的线程数
    std::cout << "Adjust threads nums = " << threads_nums << std::endl;
    for (int j = 6; j < 10; ++j) {
        pool.enqueue(j, [j] {
                std::cout << "Task " << j << " executed\n";  
                std::this_thread::sleep_for(std::chrono::seconds(1));    
        });
    }
    std::this_thread::sleep_for(std::chrono::seconds(5)); 
    tasks_size = 4;
    threads_nums = pool.set_newthread_nums(tasks_size);//下一部分任务需调整线程池的线程数
    std::cout << "Adjust threads nums = " << threads_nums << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}