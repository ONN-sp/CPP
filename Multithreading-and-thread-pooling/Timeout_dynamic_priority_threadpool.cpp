//简单线程池+动态管理线程数+任务优先级调用+任务超时处理
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
            std::vector<std::thread> threads_timeout;
            std::priority_queue<std::pair<int, std::function<void()>>, 
                                std::vector<std::pair<int, std::function<void()>>>, 
                                Compare> tasks; //使用自定义的比较器 
            std::priority_queue<std::pair<int, std::function<void()>>, 
                                std::vector<std::pair<int, std::function<void()>>>, 
                                Compare> tasks_timeout;
            std::condition_variable cond;
            std::mutex stop_mtx;
            std::mutex task_mtx;
            std::mutex task_timeout_mtx;
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
                for(auto& t: threads_timeout)//!!!
                    t.join();
            }

            template<class F, class... Args>
            void enqueue(int priority, F &&f, Args &&... arg) {//主线程
                {
                    std::unique_lock<std::mutex> lock(task_mtx);
                    tasks.emplace(priority, std::bind(std::forward<F>(f), std::forward<Args>(arg)...));
                }  
                cond.notify_one();         
            }

            //添加一个超时处理函数
            template<class Rep, class Period, class F, class... Args>
            bool enqueue_timeout(std::chrono::duration<Rep, Period> timeout_duration, int priority, F&& f, Args&&... args){
                auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
                std::future<void> future = task->get_future();
                {
                    std::unique_lock<std::mutex> lock(task_mtx);
                    tasks.emplace(priority, [task](){(*task)();});
                }
                cond.notify_one();
                //超时处理
                if(future.wait_for(timeout_duration)==std::future_status::timeout){
                    tasks_timeout.emplace(priority, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
                    return false;//超时的话就返回false,相当于是一个超时提示,也可以用异常处理来做
                }
                return true;
            }

            //对于超时任务,重新创建线程并对它们进行不超时检查运行
            void handle_timeout_task(){
                size_t size_threads = tasks_timeout.size();
                for(int i=0; i<size_threads; i++){
                    threads_timeout.emplace_back([this](){
                        std::unique_lock<std::mutex> lock(this->task_timeout_mtx);
                        std::function<void()> task_timeout;
                        task_timeout = std::move(this->tasks_timeout.top().second);
                        this->tasks_timeout.pop();
                        std::cout << "This is delayed processing alone. ";
                        task_timeout();
                    });
                }
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
    ljj::ThreadPool pool(2, 10);
    int priority = 1;
    bool task_completed = pool.enqueue_timeout(std::chrono::seconds(1), priority, [priority]{ 
        std::cout << "Task 1 with priority " << priority << " is handling." << std::endl;  
        std::this_thread::sleep_for(std::chrono::seconds(2));});
    if (task_completed)
        std::cout << "Successful!" << std::endl;
    else
        std::cout << "Timeout!" << std::endl;
    priority = 2;
    task_completed = pool.enqueue_timeout(std::chrono::seconds(3), priority, [priority]{ 
        std::cout << "Task 2 with priority " << priority << " is handling." << std::endl;  
        std::this_thread::sleep_for(std::chrono::seconds(2));});
    if (task_completed)
        std::cout << "Successful!" << std::endl;
    else
        std::cout << "Timeout!" << std::endl;
    //超时任务单独处理
    pool.handle_timeout_task();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}