#include <iostream>
#include <atomic>
#include <thread>

namespace Variable{
    int count=1;
}

void task(){
    for(int i=0;i<1000;i++){
        std::cout << Variable::count << std::endl;
        Variable::count++;
    }
}

int main(){
    std::thread T1(task);
    std::thread T2(task);
    T1.join();
    T2.join();
    return 0;
}