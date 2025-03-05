#include "block_builder.h"
#include <algorithm>
#include <cassert>
#include "../include/leveldb/comparator.h"
#include "../include/leveldb/options.h"
#include "../util/coding.h"

namespace leveldb {
    /**
     * @brief BlockBuilder类用于构建SSTable中的单个数据块
     * 
     * @param options 
     */
    BlockBuilder::BlockBuilder(const Options* options)
        : options_(options), restarts_(), counter_(0), finished_(false) 
    {
        assert(options->block_restart_interval >= 1);// 确保重启间隔配置有效（至少1次重启）
        restarts_.push_back(0); 
    }
    // 重置构建器状态，准备构建新块
    void BlockBuilder::Reset() {
        buffer_.clear();// 清空数据缓冲区
        restarts_.clear();// 清空重启点数组
        restarts_.push_back(0);// 初始化第一个重启点 
        counter_ = 0;// 重置条目计数器
        finished_ = false;// 标记块未完成
        last_key_.clear();// 清空上一个键
    }
    // 估算当前块的总大小（包括未完成的写入）
    size_t BlockBuilder::CurrentSizeEstimate() const {
        return (buffer_.size() +   // 已写入数据的大小                   
                restarts_.size() * sizeof(uint32_t) +// 重启点数组占用的空间（每个4字节） 
                sizeof(uint32_t));// 重启点数组长度的存储空间（4字节）                     
    }
    // 完成块的构建，返回最终块
    Slice BlockBuilder::Finish() {
        for (size_t i = 0; i < restarts_.size(); i++)// 将重启点偏移数组写入缓冲区
            PutFixed32(&buffer_, restarts_[i]);// 以固定4字节格式写入偏移量
        PutFixed32(&buffer_, restarts_.size());// 写入重启点数量（固定4字节）
        finished_ = true;// 标记块构建完成
        return Slice(buffer_);// 返回完整的块数据
    }
    // 添加键值对到当前块 将数据保存到各个成员变量中
    void BlockBuilder::Add(const Slice& key, const Slice& value) {
        Slice last_key_piece(last_key_);// last_key保存上一个加入的键
        assert(!finished_);// 确保块未完成
        assert(counter_ <= options_->block_restart_interval);// 条目数不超过重启间隔
        assert(buffer_.empty() || options_->comparator->Compare(key, last_key_piece) > 0);// 确保键按顺序添加（第一个键无需比较，或新键必须大于上一个键）
        size_t shared = 0;// shared用来保存本次加入的键和上一个加入的键的共同前缀长度
        if (counter_ < options_->block_restart_interval) {// 查看当前已经插入的键值对数量是否超过16
            const size_t min_length = std::min(last_key_piece.size(), key.size());// 计算两个键的最小长度
            while ((shared < min_length) && (last_key_piece[shared] == key[shared]))// 逐字节比较，直到发现差异字符
                shared++;
        } 
        else {// 达到重启间隔时，添加新重启点
            restarts_.push_back(buffer_.size());// 记录当前缓冲区位置
            counter_ = 0;// 重置条目计数器
        }
        const size_t non_shared = key.size() - shared;// 计算非共享部分长度
        // 将编码头写入缓冲区buffer_
        PutVarint32(&buffer_, shared);// 共享前缀长度
        PutVarint32(&buffer_, non_shared);// 非共享部分长度
        PutVarint32(&buffer_, value.size());// 值长度
        buffer_.append(key.data() + shared, non_shared);// 追加键的非共享部分
        buffer_.append(value.data(), value.size());// 追加完整值内容
        last_key_.resize(shared);
        last_key_.append(key.data() + shared, non_shared);// 更新last_key_为当前完整键（用于下次比较）
        assert(Slice(last_key_) == key);
        counter_++;// 递增当前重启区间条目计数
    }
}