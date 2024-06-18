#ifndef MUTEX_H
#define MUTEX_H

#include <mutex>
#include "NonCopyable.h"

namespace tiny_muduo{
    class MutexLock : public NonCopyAble{
        public:
              MutexLock() = default;
              ~MutexLock() = default;
              void lock() {
                   mutex_.lock();
              }
              void unlock() {
                   mutex_.unlock();
              }
              std::mutex& native_handle() { // 提供访问底层std::mutex的接口
                   return mutex_;
              }
        private:
            std::mutex mutex_;  // 使用 C++11 的 std::mutex
    };
    // C++11风格的MutexLockGuard类,这个类等价于std::lock_guard
    class MutexLockGuard : public NonCopyAble {
            public:
                explicit MutexLockGuard(MutexLock& mutex)
                         : mutex_(mutex.native_handle()) { // 使用传入的MutexLock中的std::mutex
                           mutex_.lock();
                }
                ~MutexLockGuard() {
                    mutex_.unlock();
                }
            private:
                std::mutex& mutex_;
    };
}
#endif