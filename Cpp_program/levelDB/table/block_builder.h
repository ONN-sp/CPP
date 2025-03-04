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
            const Options* options_;
            std::string buffer_;          
            std::vector<uint32_t> restarts_;
            int counter_;                   
            bool finished_; 
            std::string last_key_;
    };
}
#endif