#ifndef CONDITION_H
#define CONDITION_H

#include <condition_variable>
#include "MutexLock.h"
#include <chrono>
#include "NonCopyAble.h"

namespace tiny_muduo {

class Condition : public NonCopyAble {
 public:
  explicit Condition(MutexLock& mutex) : mutex_(mutex) {}

  ~Condition() = default;

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_.mutex(), std::defer_lock);//创建一个std::unique_lock,但是不上锁  因为在asynclogging的ThreadFunc一开始就上锁了,如果这还上锁那么就会死锁
    cond_.wait(lock);
  }

  bool WaitForFewSeconds(std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(mutex_.mutex(), std::defer_lock);
    return cond_.wait_for(lock, duration) == std::cv_status::no_timeout;
  }

  void Notify() {
    cond_.notify_one();
  }

  void NotifyAll() {
    cond_.notify_all();
  }

 private:
  MutexLock& mutex_;
  std::condition_variable cond_;
}; 

}
#endif