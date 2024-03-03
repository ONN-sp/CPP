#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>
#include <future>
#include <functional>

namespace ljj{
    //通过class重载运算符()
    class Compare{
        public:
            bool operator()(const std::pair<int, std::function<void()>> &ls, const std::pair<int, std::function<void()>> &rs){
                    return ls.first>rs.first;//实现小顶堆
                }
    };

    class ThreadPool {
        private:
            std::vector<std::thread> threads;
            std::priority_queue<std::pair<int, std::function<void()>>, 
                                std::vector<std::pair<int, std::function<void()>>>, 
                                Compare> tasks; //使用自定义的比较器
            std::condition_variable cond;
            std::mutex stop_mtx;
            bool stop;

        public:
            ThreadPool(size_t thread_nums) : stop(false){
                for (int i = 0; i < thread_nums; i++) {
                    threads.emplace_back([this]() {
                        while (true) {
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(this->stop_mtx);
                                this->cond.wait(lock, [this]() { return this->stop || !this->tasks.empty(); });
                                if (this->stop && this->tasks.empty())
                                    return;
                                std::this_thread::sleep_for(std::chrono::seconds(3));//确保优先级不同的任务都添加到了任务队列,模拟实际情况所用而已
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

            //使用std::future
            template<class F, class... Args>
            //定义一个返回为std::future类型对象的任务函数,这样方便异步获取结果(而不需要全局变量).但是对于本程序其实用不用都无所谓
            auto enqueue(int priority, F &&f, Args &&... args)-> std::future<decltype(f(args...))>{
                using return_type = decltype(f(args...));
                auto task=std::make_shared<std::packaged_task<return_type()>> (std::bind(std::forward<F>(f), std::forward<Args>(args)...));
                std::future<return_type> result = task->get_future();
                {
                    std::unique_lock<std::mutex> lock(stop_mtx);
                    tasks.emplace(priority, [task](){(*task)();});//std::make_shared返回的是一个智能指针,该指针管理一个动态分配的std::packaged_task<return_type()>对象
                }
                cond.notify_one();
                return result;
            }

            //不使用std::future
            // template<class F, class... Args>
            // void enqueue(int priority, F &&f, Args &&... arg) {
            //     {
            //         std::unique_lock<std::mutex> lock(stop_mtx);
            //         tasks.emplace(priority, std::bind(std::forward<F>(f), std::forward<Args>(arg)...));
            //     }
            //     cond.notify_one();
            // }
    };
};

int main() {
    // 创建线程池,初始线程数为2
    ljj::ThreadPool pool(2);
    std::this_thread::sleep_for(std::chrono::seconds(2));//确保线程池在任务执行前添加完毕
    // 提交三个任务到线程池,分别指定优先级,数值越小优先级越高
    pool.enqueue(3, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "Low priority task done\n"; });
    pool.enqueue(2, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "Medium priority task done\n"; });
    pool.enqueue(1, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "High priority task done\n"; });
    // 等待所有任务执行完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
