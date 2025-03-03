#ifndef LEVELDB_DB_LOG_WRITER_H_
#define LEVELDB_DB_LOG_WRITER_H_

#include <cstdint>
#include "log_format.h"
#include "../inlcude/leveldb/slice.h"
#include "../inlcude/leveldb/status.h"

namespace leveldb {
    class WritableFile;
    namespace log {
    class Writer {
        public:
            explicit Writer(WritableFile* dest);
            Writer(WritableFile* dest, uint64_t dest_length);
            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;
            ~Writer();
            Status AddRecord(const Slice& slice);
        private:
            Status EmitPhysicalRecord(RecordType type, const char* ptr, size_t length);
            WritableFile* dest_;
            int block_offset_; 
            uint32_t type_crc_[kMaxRecordType + 1];
        };
    }
}
#endif