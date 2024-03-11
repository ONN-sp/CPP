#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i)
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
    }

    template<class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

    // 添加具有超时功能的任务到线程池
    template<class Rep, class Period, class F, class... Args>
    bool enqueueWithTimeout(const std::chrono::duration<Rep, Period>& timeout_duration, F&& f, Args&&... args) {
        std::atomic<bool> task_completed(false); // 任务完成标志
        std::shared_ptr<std::thread> task_thread; // 任务线程
        std::condition_variable task_cv; // 用于等待任务完成的条件变量
        std::mutex task_mutex; // 用于保护任务完成标志的互斥量

        // 启动任务线程
        task_thread = std::make_shared<std::thread>([&] {
            std::shared_ptr<std::packaged_task<void()>> task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            (*task)();
            {
                std::unique_lock<std::mutex> lock(task_mutex);
                task_completed.store(true);
            }
            task_cv.notify_one(); // 通知等待的线程任务已完成
        });

        // 等待任务完成或超时
        std::unique_lock<std::mutex> lock(task_mutex);
        if (!task_cv.wait_for(lock, timeout_duration, [&]() { return task_completed.load(); })) {
            // 超时处理：终止任务线程
            if (task_thread && task_thread->joinable()) {
                task_thread->detach(); // 分离任务线程，以防止内存泄漏
                task_thread.reset(); // 释放任务线程资源
            }
            return false; // 超时
        }
        return true; // 任务完成
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// 测试函数
void print_num(int num) {
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟长时间任务
    std::cout << "Number: " << num << std::endl;
}

int main() {
    ThreadPool pool(4); // 创建包含4个线程的线程池

    // 添加多个任务到线程池
    for (int i = 0; i < 8; ++i) {
        pool.enqueue(print_num, i);
    }

    // 等待一会，以便观察任务执行情况
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 添加具有超时功能的任务到线程池
    bool task_completed = pool.enqueueWithTimeout(std::chrono::seconds(1), [] { std::this_thread::sleep_for(std::chrono::seconds(2)); });
    if (task_completed)
        std::cout << "任务在超时时间内完成。" << std::endl;
    else
        std::cout << "任务超时。" << std::endl;

    return 0;
}
