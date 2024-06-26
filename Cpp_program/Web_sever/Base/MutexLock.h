#ifndef MUTEX_H
#define MUTEX_H

#include <mutex>
#include "NonCopyAble.h"

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
              std::mutex& mutex(){
                return mutex_;
              }
        private:
            std::mutex mutex_;
    };
    class MutexLockGuard : public NonCopyAble {
            public:
                explicit MutexLockGuard(MutexLock& mutex)
                         : mutex_(mutex) {
                           mutex_.lock();
                }
                ~MutexLockGuard() {
                    mutex_.unlock();
                }
            private:
                MutexLock& mutex_;
    };
}
#endif