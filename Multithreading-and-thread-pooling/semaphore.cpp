//C++20,信号量的简单实例
#include <iostream>
#include <thread>
#include <semaphore>//c++20才有

std::counting_semaphore<1> sem(1); // 初始化信号量，允许一个线程同时访问

void worker(int id) {
    sem.acquire(); // 获取信号量
    std::cout << "Worker " << id << " is working" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    sem.release(); // 释放信号量
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();

    return 0;
}
