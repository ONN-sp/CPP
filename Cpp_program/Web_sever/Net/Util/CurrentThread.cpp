#include "CurrentThread.h"

using namespace CurrentThread;

__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;


void CacheTid(){return;};// 这个函数负责缓存当前线程的线程 ID
// 获取当前线程的线程 ID
inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0))//__builtin_expect是一种底层优化 相当于一个断言,_cachedTid == 0表示还没有获取到这个Tid
        CacheTid();
    return t_cachedTid;
}
inline const char* tid2string() { 
    return t_formattedTid; } 
inline int tidstringlength() { return t_formattedTidLength; }
