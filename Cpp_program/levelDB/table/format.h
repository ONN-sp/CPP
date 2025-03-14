#ifndef LEVELDB_TABLE_FORMAT_H_
#define LEVELDB_TABLE_FORMAT_H_

#include <cstdint>
#include <string>
#include "../include/leveldb/slice.h"
#include "../include/leveldb/status.h"
#include "../include/leveldb/table_builder.h"

namespace leveldb {
    class Block;
    class RandomAccessFile;
    struct ReadOptions;
    class BlockHandle {
        public:
            enum { kMaxEncodedLength = 10 + 10 };
            BlockHandle();
            uint64_t offset() const { return offset_; }
            void set_offset(uint64_t offset) { offset_ = offset; }
            uint64_t size() const { return size_; }
            void set_size(uint64_t size) { size_ = size; }
            void EncodeTo(std::string* dst) const;
            Status DecodeFrom(Slice* input);
        private:
            uint64_t offset_;
            uint64_t size_;
    };
    class Footer {
        public:
            enum { kEncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8 };
            Footer() = default;
            const BlockHandle& metaindex_handle() const { return metaindex_handle_; }
            void set_metaindex_handle(const BlockHandle& h) { metaindex_handle_ = h; }
            const BlockHandle& index_handle() const { return index_handle_; }
            void set_index_handle(const BlockHandle& h) { index_handle_ = h; }
            void EncodeTo(std::string* dst) const;
            Status DecodeFrom(Slice* input);
        private:
            BlockHandle metaindex_handle_;
            BlockHandle index_handle_;
    };
    static const uint64_t kTableMagicNumber = 0xdb4775248b80fb57ull;
    static const size_t kBlockTrailerSize = 5;
    struct BlockContents {
        Slice data;         
        bool cachable;       
        bool heap_allocated; 
    };
    Status ReadBlock(RandomAccessFile* file, const ReadOptions& options, const BlockHandle& handle, BlockContents* result);
    inline BlockHandle::BlockHandle()
        : offset_(~static_cast<uint64_t>(0)), size_(~static_cast<uint64_t>(0)) 
    {}
}
#endif