#ifndef LEVELDB_INCLUDE_CACHE_H_
#define LEVELDB_INCLUDE_CACHE_H_

#include <cstdint>
#include "export.h"
#include "slice.h"

namespace leveldb {
    class LEVELDB_EXPORT Cache;
    LEVELDB_EXPORT Cache* NewLRUCache(size_t capacity);
    class LEVELDB_EXPORT Cache {
        public:
            Cache() = default;
            Cache(const Cache&) = delete;
            Cache& operator=(const Cache&) = delete;
            virtual ~Cache();
            virtual Handle* Insert(const Slice& key, void* value, size_t charge, void (*deleter)(const Slice& key, void* value)) = 0;
            virtual Handle* Lookup(const Slice& key) = 0;
            virtual void Release(Handle* handle) = 0;
            virtual void* Value(Handle* handle) = 0;
            virtual void Erase(const Slice& key) = 0;
            virtual uint64_t NewId() = 0;
            virtual void Prune() {}
            virtual size_t TotalCharge() const = 0;
    };
}
#endif