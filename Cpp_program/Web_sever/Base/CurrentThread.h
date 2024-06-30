#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

#include <unistd.h>

namespace tiny_muduo {
extern __thread int t_cachedTid; // 缓存的线程 ID
extern __thread char t_formattedTid[32];// 字符串形式的线程ID
extern __thread int t_formattedTidLength;// 线程ID的长度

void CacheTid();// 这个函数负责缓存当前线程的线程 ID

// 获取当前线程的线程 ID
inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0))//__builtin_expect是一种底层优化 相当于一个断言,_cachedTid == 0表示还没有获取到这个Tid
        CacheTid();
    return t_cachedTid;
}
inline const char* tid2string() { 
    return t_formattedTid; 
} 
inline int tidstringlength() { 
    return t_formattedTidLength; 
}
}
#endif
