# Multithreading-and-thread-pooling
1. learn.md是学习多线程和线程池的日志
2. atmoic.cpp、condition_variable.cpp、semaphore.cpp分别是原子操作、条件变量和信号量的简单学习例子
3. `<semaphore>` C++20才有
4. Easy_Threadpool.cpp是无多余功能的线程池
5. Priority_Threadpool.cpp是添加了任务优先级调度功能的线程池
6. Dynamic_priority_threadpool.cpp是添加了任务优先级调度功能+动态管理线程(可以依据此时的所需的任务数调整线程池的线程数)功能的线程池
7. Timeout_dynamic_priority_threadpool.cpp是添加了任务优先级调度功能+动态管理线程+任务超时处理(超时任务在主线程给出提示,并给出了一个对于超时任务处理的函数,即不检查超时,重新创建线程运行所有前面的超时任务)
