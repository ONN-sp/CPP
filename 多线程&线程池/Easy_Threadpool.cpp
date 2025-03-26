//简单线程池(无任务优先级、动态管理线程池等功能)
#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>
#include <functional>

namespace ljj{
    class ThreadPool{
        private:
            std::vector<std::thread> threads;
            std::queue<std::function<void()>> tasks;
            std::condition_variable cond;
            std::mutex stop_mtx;
            std::mutex task_mtx;
            bool stop;
        public:
            ThreadPool(size_t thread_nums):stop(false){//构造函数:向线程池添加线程(线程里面要利用条件变量控制实现任务执行).在这个构造函数中并没有直接调用std::thread的构造函数来创建线程,而是通过emplace_back()函数以lambda函数作为参数来实现线程的创建
                for(int i=0;i<thread_nums;i++){
                    threads.emplace_back([this](){//threads是一个元素为std::thread的数组,因此这个lambda函数进来会调用thread的构造函数默认构造一个thread变量,而这个thread线程的入口函数就是这个lambda表达式
                        while(1){//线程不能终止,要一直执行任务队列的任务
                            std::function<void()> task;//因为后续用的是std::bind(),所以这个函数包装器中的参数可以为空
                            {
                            std::unique_lock<std::mutex> lock(task_mtx);
                            this->cond.wait(lock, [this](){return !tasks.empty() || this->stop;});// 任务非空或stop触发就不用wait
                            if(this->stop || tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                            }    
                            task();//task任务的执行不能被锁住,如果锁住那么就可能会导致某个线程一直执行任务,而其它线程被锁住
                        }
                    });
                }
            }

            ~ThreadPool(){//析构函数中会等作用域结束自动调用
                {
                    std::unique_lock<std::mutex> lock(stop_mtx);
                    stop = true;//代表这个线程池结束了  
                }
                cond.notify_all();
                for(auto& t:threads)
                    t.join();
            }

            template <class F, class... Args>
            void enqueue(F &&f, Args&&... arg){//任务队列函数(向队列中加任务).函数模板里面右值引用=万能引用
                {
                    std::unique_lock<std::mutex> lock(task_mtx);
                    tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(arg)...));//std::bind将函数和函数参数绑定,std::forward表示完美转化(也转化为万能引用)
                }
                this->cond.notify_one();//任务队列中加了任务,通知线程数组中的线程去执行
            }
    };
};

// 示例任务函数
void taskFunction(int id) {
    std::cout << "Task " << id << " is being processed by thread " << std::endl;
    auto idd = std::this_thread::get_id();
    std::cout << idd << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // 模拟任务执行时间
    std::cout << "Task " << id << " has been completed by thread " << std::endl;
    std::cout << idd << std::endl;
}

int main() {
    //线程数组维护4个线程
    ljj::ThreadPool pool(4);
    //向线程池中提交一些任务
    for (int i = 0; i < 8; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        pool.enqueue(taskFunction, i);
    }
    // 等待所有任务完成
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}


   
   