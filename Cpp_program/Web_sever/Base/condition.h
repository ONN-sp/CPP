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
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock);
  }

  template <typename Duration>
  bool WaitFor(Duration duration) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cond_.wait_for(lock, duration) == std::cv_status::no_timeout;
  }

  bool WaitForFewSeconds(double seconds) {
    return WaitFor(std::chrono::duration<double>(seconds));
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