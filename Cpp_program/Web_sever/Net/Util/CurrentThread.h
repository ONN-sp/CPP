#ifndef TINY_MUDUO_CURRENTTHREAD_H_
#define TINY_MUDUO_CURRENTTHREAD_H_

#include <unistd.h>

namespace CurrentThread {
extern __thread int t_cachedTid; // 缓存的线程 ID
extern __thread char t_formattedTid[32];// 字符串形式的线程ID
extern __thread int t_formattedTidLength;// 线程ID的长度
void CacheTid();
inline int tid();
inline const char* tid2string();
inline int tidstringlength();
}
#endif
