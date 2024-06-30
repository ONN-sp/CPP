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
    std::unique_lock<MutexLock> lock(mutex_.mutex());
    cond_.wait(lock);
  }

  bool WaitForFewSeconds(std::chrono::seconds duration) {
    std::unique_lock<MutexLock> lock(mutex_.mutex());
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