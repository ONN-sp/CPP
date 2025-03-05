#ifndef LEVELDB_TABLE_BLOCK_BUILDER_H_
#define LEVELDB_TABLE_BLOCK_BUILDER_H_

#include <cstdint>
#include <vector>
#include "../include/leveldb/slice.h"

namespace leveldb {
    struct Options;
    class BlockBuilder {
        public:
            explicit BlockBuilder(const Options* options);
            BlockBuilder(const BlockBuilder&) = delete;
            BlockBuilder& operator=(const BlockBuilder&) = delete;
            void Reset();
            void Add(const Slice& key, const Slice& value);
            Slice Finish();
            size_t CurrentSizeEstimate() const;
            bool empty() const { return buffer_.empty(); }
        private:
            const Options* options_;// 存储配置选项（如重启点对应的键值对间隔）
            std::string buffer_;// 块的内容,所有的键值对都保存到buffer_中
            std::vector<uint32_t> restarts_;// 存储重启点偏移的数组
            int counter_;// 当前重启区间内的条目计数                  
            bool finished_;// 指明是否已经调用了Finish() 
            std::string last_key_;// 上一个保存的键,当加入新键时,用来计算和上一个键的共同前缀部分
    };
}
#endif