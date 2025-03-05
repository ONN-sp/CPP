#ifndef LEVELDB_INCLUDE_TABLE_H_
#define LEVELDB_INCLUDE_TABLE_H_

#include <cstdint>
#include "export.h"
#include "iterator.h"

namespace leveldb {
    class Block;
    class BlockHandle;
    class Footer;
    struct Options;
    class RandomAccessFile;
    struct ReadOptions;
    class TableCache;
    class LEVELDB_EXPORT Table {
        public:
            static Status Open(const Options& options, RandomAccessFile* file,
                                uint64_t file_size, Table** table);
            Table(const Table&) = delete;
            Table& operator=(const Table&) = delete;
            ~Table();
            Iterator* NewIterator(const ReadOptions&) const;
            uint64_t ApproximateOffsetOf(const Slice& key) const;
        private:
            friend class TableCache;
            struct Rep;
            static Iterator* BlockReader(void*, const ReadOptions&, const Slice&);
            explicit Table(Rep* rep) : rep_(rep) {}
            Status InternalGet(const ReadOptions&, const Slice& key, void* arg,
                                void (*handle_result)(void* arg, const Slice& k,
                                                    const Slice& v));
            void ReadMeta(const Footer& footer);
            void ReadFilter(const Slice& filter_handle_value);
            Rep* const rep_;
    };
}  
#endif 