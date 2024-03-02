#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>
#include <functional>


    class Compare {
    public:
        bool operator()(const std::pair<int, std::function<void()>> &lhs, 
                        const std::pair<int, std::function<void()>> &rhs) const {
            return lhs.first > rhs.first; // 降序比较，以确保优先级高的任务先执行
                        }
    };

    class ThreadPool {
        private:
            std::vector<std::thread> threads;
            std::priority_queue<std::pair<int, std::function<void()>>, 
                                std::vector<std::pair<int, std::function<void()>>>, 
                                Compare> tasks; // 使用自定义的比较器
            std::condition_variable cond;
            std::mutex stop_mtx;
            bool stop;

        public:
            ThreadPool(size_t thread_nums) : stop(false) {
                for (int i = 0; i < thread_nums; i++) {
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
            void enqueue(int priority, F &&f, Args &&... arg) {
                {
                    std::unique_lock<std::mutex> lock(stop_mtx);
                    tasks.emplace(priority, std::bind(std::forward<F>(f), std::forward<Args>(arg)...));
                }
                cond.notify_one();
            }
    };

int main() {
    // 创建线程池，初始线程数为2
    ThreadPool pool(1);

    // 提交三个任务到线程池，分别指定优先级
    pool.enqueue(1, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "Low priority task done\n"; });
    pool.enqueue(5, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "Medium priority task done\n"; });
    pool.enqueue(2, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "High priority task done\n"; });

    // 等待所有任务执行完成
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
