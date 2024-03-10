#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <algorithm>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        
                        if (stop && tasks.empty())
                            return;
                        
                        // Get the highest priority task
                        task = std::move(tasks.top().second);
                        tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(int priority, F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            
            tasks.emplace(priority, [task]() { (*task)(); });
        }
        
        condition.notify_one();
        return res;
    }

    auto set_num_threads(size_t numThreads) {
        if (numThreads == 0)
            throw std::invalid_argument("Number of threads must be greater than 0");

        size_t currentNumThreads = workers.size();
        if (numThreads > currentNumThreads) {
            // Increase the number of threads
            for (size_t i = currentNumThreads; i < numThreads; ++i) {
                workers.emplace_back([this] {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);
                            condition.wait(lock, [this] { return stop || !tasks.empty(); });
                            
                            if (stop && tasks.empty())
                                return;
                            
                            task = std::move(tasks.top().second);
                            tasks.pop();
                        }

                        task();
                    }
                });
            }
        } else if (numThreads < currentNumThreads) {
            // Decrease the number of threads
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            
            condition.notify_all();
            
            for (std::thread& worker : workers)
                worker.join();
            
            workers.erase(workers.begin() + numThreads, workers.end());

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = false;
            }

            for (size_t i = numThreads; i < currentNumThreads; ++i) {
                workers.emplace_back([this] {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);
                            condition.wait(lock, [this] { return stop || !tasks.empty(); });
                            
                            if (stop && tasks.empty())
                                return;
                            
                            task = std::move(tasks.top().second);
                            tasks.pop();
                        }

                        task();
                    }
                });
            }
        }
        return workers.size();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        
        condition.notify_all();
        
        for (std::thread& worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::priority_queue<std::pair<int, std::function<void()>>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};

int main() {
    ThreadPool pool(4);

    // Enqueue tasks with different priorities
    pool.enqueue(1, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "Low priority task done\n"; });
    pool.enqueue(2, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "Medium priority task done\n"; });
    pool.enqueue(3, []{ std::this_thread::sleep_for(std::chrono::seconds(1)); std::cout << "High priority task done\n"; });

    // Wait for tasks to finish
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Change number of threads
    size_t thread_nums = pool.set_num_threads(2);
    std::cout << "Adjust threads nums = " << thread_nums << std::endl;
    // Wait for tasks to finish
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
