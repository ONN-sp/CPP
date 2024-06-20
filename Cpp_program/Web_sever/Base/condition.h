#ifndef TINY_MUDUO_CONDITION_H_
#define TINY_MUDUO_CONDITION_H_

#include <condition_variable>
#include <mutex>
#include <chrono>

#include "NonCopyAble.h"
namespace tiny_muduo {

class Condition : public NonCopyAble {
 public:
  explicit Condition(std::mutex& mutex) : mutex_(mutex) {}

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
  std::mutex& mutex_;
  std::condition_variable cond_;
}; 

}
#endif