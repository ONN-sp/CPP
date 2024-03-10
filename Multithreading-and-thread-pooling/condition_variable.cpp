//利用条件变量+互斥锁解决一个生产者消费者模型
#include <iostream>
#include <mutex>
#include <thread>
#include <queue>
#include <random>
#include <condition_variable>

std::queue<int> g_queue;
std::condition_variable g_cv;
std::mutex mtx;//g_queue共享资源
void producer(){
    //生产者
    std::random_device rd;
    std::mt19937 gen(rd());//随机数引擎
    std::uniform_int_distribution<int> dis(1, 10);//1-10的整数
    int i=0;
    while(1){
        std::unique_lock<std::mutex> lock(mtx);
        g_queue.emplace(i);
        //通知消费者来取任务
        g_cv.notify_one();
        std::cout << "Producer : " << i << std::endl;
        i++;
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
    }
}

void consumer(){
    //消费者
    std::random_device rd2;
    std::mt19937 gen2(rd2());//随机数引擎
    std::uniform_int_distribution<int> dis2(1, 5);
    while(1){
        std::unique_lock<std::mutex> lock(mtx);
        //队列为空就要等待
        g_cv.wait(lock, [](){return !g_queue.empty();});//cv.wait(lock, pred),pred是一个可调用对象,通常是一个函数对象(函数指针、Lambda表达式等).pred是判断线程是否需要等待的条件,如果pred返回false则wait函数继续等待;如果pred返回true则会直接不等待
        int value = g_queue.front();
        g_queue.pop();
        std::cout << "Consumer :" << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(dis2(gen2)));
    }
}

int main(){
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join();
    t2.join();
     return 0;
}