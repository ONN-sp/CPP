#ifndef TINY_MUDUO_CURRENTTHREAD_H_
#define TINY_MUDUO_CURRENTTHREAD_H_

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread {
extern __thread int t_cachedTid; // 缓存的线程 ID

// 这个函数负责缓存当前线程的线程 ID
void CacheTid();

// 获取当前线程的线程 ID
inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0))//__builtin_expect是一种底层优化 相当于一个断言,_cachedTid == 0表示还没有获取到这个Tid
        CacheTid();
    return t_cachedTid;
}
}
#endif
