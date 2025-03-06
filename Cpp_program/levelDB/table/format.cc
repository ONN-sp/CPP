#include "format.h"
#include "../include/leveldb/env.h"
#include "../include/leveldb/options.h"
#include "../port/port.h"
#include "block.h"
#include "../util/coding.h"
#include "../util/crc32c.h"

namespace leveldb {
    void BlockHandle::EncodeTo(std::string* dst) const {
        assert(offset_ != ~static_cast<uint64_t>(0));
        assert(size_ != ~static_cast<uint64_t>(0));
        PutVarint64(dst, offset_);
        PutVarint64(dst, size_);
    }
    Status BlockHandle::DecodeFrom(Slice* input) {
        if (GetVarint64(input, &offset_) && GetVarint64(input, &size_))
            return Status::OK();
        else 
            return Status::Corruption("bad block handle");
    }
    void Footer::EncodeTo(std::string* dst) const {
        const size_t original_size = dst->size();
        metaindex_handle_.EncodeTo(dst);
        index_handle_.EncodeTo(dst);
        dst->resize(2 * BlockHandle::kMaxEncodedLength);
        PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber & 0xffffffffu));
        PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber >> 32));
        assert(dst->size() == original_size + kEncodedLength);
        (void)original_size; 
    }
    Status Footer::DecodeFrom(Slice* input) {
        if (input->size() < kEncodedLength)
            return Status::Corruption("not an sstable (footer too short)");
        const char* magic_ptr = input->data() + kEncodedLength - 8;
        const uint32_t magic_lo = DecodeFixed32(magic_ptr);
        const uint32_t magic_hi = DecodeFixed32(magic_ptr + 4);
        const uint64_t magic = ((static_cast<uint64_t>(magic_hi) << 32) | (static_cast<uint64_t>(magic_lo)));
        if (magic != kTableMagicNumber)
            return Status::Corruption("not an sstable (bad magic number)");
        Status result = metaindex_handle_.DecodeFrom(input);
        if (result.ok()) 
            result = index_handle_.DecodeFrom(input);
        if (result.ok()) {
            const char* end = magic_ptr + 8;
            *input = Slice(end, input->data() + input->size() - end);
        }
        return result;
    }
    Status ReadBlock(RandomAccessFile* file, const ReadOptions& options, const BlockHandle& handle, BlockContents* result) {
        result->data = Slice();
        result->cachable = false;
        result->heap_allocated = false;
        size_t n = static_cast<size_t>(handle.size());
        char* buf = new char[n + kBlockTrailerSize];
        Slice contents;
        Status s = file->Read(handle.offset(), n + kBlockTrailerSize, &contents, buf);
        if (!s.ok()) {
            delete[] buf;
            return s;
        }
        if (contents.size() != n + kBlockTrailerSize) {
            delete[] buf;
            return Status::Corruption("truncated block read");
        }
        const char* data = contents.data();  
        if (options.verify_checksums) {
            const uint32_t crc = crc32c::Unmask(DecodeFixed32(data + n + 1));
            const uint32_t actual = crc32c::Value(data, n + 1);
            if (actual != crc) {
                delete[] buf;
                s = Status::Corruption("block checksum mismatch");
                return s;
            }
        }
        switch (data[n]) {
            case kNoCompression:
                if (data != buf) {
                    delete[] buf;
                    result->data = Slice(data, n);
                    result->heap_allocated = false;
                    result->cachable = false; 
                } 
                else {
                    result->data = Slice(buf, n);
                    result->heap_allocated = true;
                    result->cachable = true;
                }
                break;
            case kSnappyCompression: {
                size_t ulength = 0;
                if (!port::Snappy_GetUncompressedLength(data, n, &ulength)) {
                    delete[] buf;
                    return Status::Corruption("corrupted snappy compressed block length");
                }
                char* ubuf = new char[ulength];
                if (!port::Snappy_Uncompress(data, n, ubuf)) {
                    delete[] buf;
                    delete[] ubuf;
                    return Status::Corruption("corrupted snappy compressed block contents");
                }
                delete[] buf;
                result->data = Slice(ubuf, ulength);
                result->heap_allocated = true;
                result->cachable = true;
                break;
                }
            case kZstdCompression: {
                size_t ulength = 0;
                if (!port::Zstd_GetUncompressedLength(data, n, &ulength)) {
                    delete[] buf;
                    return Status::Corruption("corrupted zstd compressed block length");
                }
                char* ubuf = new char[ulength];
                if (!port::Zstd_Uncompress(data, n, ubuf)) {
                    delete[] buf;
                    delete[] ubuf;
                    return Status::Corruption("corrupted zstd compressed block contents");
                }
                delete[] buf;
                result->data = Slice(ubuf, ulength);
                result->heap_allocated = true;
                result->cachable = true;
                break;
            }
            default:
            delete[] buf;
            return Status::Corruption("bad block type");
        }
        return Status::OK();
    }
} 