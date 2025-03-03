#ifndef LEVELDB_UTIL_ARENA_H_
#define LEVELDB_UTIL_ARENA_H_

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace leveldb {
    /**
     * @brief 构建Arena内存池，用于高效管理内存分配，减少小对象频繁分配的开销
     * 
     */
    class Arena {
        public:
            Arena();
            Arena(const Arena&) = delete;
            Arena& operator=(const Arena&) = delete;
            ~Arena();
            char* Allocate(size_t bytes);// 分配未对齐的内存块
            char* AllocateAligned(size_t bytes);// 分配对齐的内存块,即不仅要分配指定字节大小的内存,还要保证内存首地址满足内存对齐的相关原则
            size_t MemoryUsage() const {// 返回Arena内存池分配的总体内存空间大小
                return memory_usage_.load(std::memory_order_relaxed);
            }
        private:
            char* AllocateFallback(size_t bytes);// 当当前内存块剩余空间不足时的后备分配策略
            char* AllocateNewBlock(size_t block_bytes);// 分配新的内存块并记录
            char* alloc_ptr_;// 指向当前最新Block中空闲内存空间的起始地址
            size_t alloc_bytes_remaining_;// 当前Block所剩余的空间内存空间大小
            std::vector<char*> blocks_;// 每一个指针指向一个堆空间中预分配好的内存块
            std::atomic<size_t> memory_usage_;// 存储当前Arena内存池所申请的总共的内存空间大小
    };
    /**
     * @brief 分配指定大小的内存空间
     * 
     * @param bytes 
     * @return char* 
     */
    inline char* Arena::Allocate(size_t bytes) {
        assert(bytes > 0);
        if (bytes <= alloc_bytes_remaining_) {
            char* result = alloc_ptr_;
            alloc_ptr_ += bytes;
            alloc_bytes_remaining_ -= bytes;
            return result;
        }
        return AllocateFallback(bytes);
    }
}
#endif