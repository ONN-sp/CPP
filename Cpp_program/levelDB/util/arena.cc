#include "arena.h"

namespace leveldb {
    static const int kBlockSize = 4096;// 默认Block大小,4KB
    Arena::Arena()
        : alloc_ptr_(nullptr), alloc_bytes_remaining_(0), memory_usage_(0) {}
    /**
     * @brief 通过对block的vector数组的遍历,进而删除其分配的所有Block所对应的指针,即一次性全部释放
     * 
     */
    Arena::~Arena() {
        for (size_t i = 0; i < blocks_.size(); i++)
            delete[] blocks_[i];
    }
    /**
     * @brief 处理大对象或当前块空间不足的情况
     * 
     * @param bytes 
     * @return char* 
     */
    char* Arena::AllocateFallback(size_t bytes) {
        if (bytes > kBlockSize / 4) {// 如果请求大小超过块大小的1/4(1KB)，单独分配独立块
            char* result = AllocateNewBlock(bytes);// 直接分配独立块，避免浪费大块内存
            return result;
        }
        // 否则分配新块（默认块大小），并从中分配
        alloc_ptr_ = AllocateNewBlock(kBlockSize);// 分配新块
        alloc_bytes_remaining_ = kBlockSize;// 重置剩余空间
        char* result = alloc_ptr_;
        alloc_ptr_ += bytes;// 移动分配指针
        alloc_bytes_remaining_ -= bytes;// 减少剩余空间
        return result;
    }
    /**
     * @brief 对齐内存分配,AllocateAligned进行内存分配所返回的起始地址应为(64位系统为例)8字节倍数,即可能实际分配的内存更大(为了能对齐)
     * 
     * @param bytes 
     * @return char* 
     */
    char* Arena::AllocateAligned(size_t bytes) {
        const int align = (sizeof(void*) > 8) ? sizeof(void*) : 8;// 确定对齐要求：至少8字节或指针大小（取较大者，且必须是2的幂）
        static_assert((align & (align - 1)) == 0, "Pointer size should be a power of 2");// 确保这个需要对齐的值是2的正整数幂
        size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align - 1);// 计算当前分配指针的对齐偏移量,用位运算实现
        size_t slop = (current_mod == 0 ? 0 : align - current_mod);// 需要填补的字节数
        size_t needed = bytes + slop;// 实际需要的内存总量
        char* result;
        if (needed <= alloc_bytes_remaining_) {// 当前块剩余空间足够：调整指针并分配
            result = alloc_ptr_ + slop;// 跳过填补字节
            alloc_ptr_ += needed;// 移动分配指针
            alloc_bytes_remaining_ -= needed;// 更新剩余空间
        } 
        else // 空间不足：走后备分配路径（可能分配新块）
            result = AllocateFallback(bytes);
        assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);// 验证结果地址的对齐正确性
        return result;
    }
    /**
     * @brief 分配新内存块并登记到内存池管理中
     * 
     * @param block_bytes 
     * @return char* 
     */
    char* Arena::AllocateNewBlock(size_t block_bytes) {
        char* result = new char[block_bytes];// 分配新内存块
        blocks_.push_back(result);// 记录块指针到列表中
        memory_usage_.fetch_add(block_bytes + sizeof(char*), std::memory_order_relaxed);// 原子操作更新总内存使用量（块大小 + 指针存储开销）
        return result;
    }
}