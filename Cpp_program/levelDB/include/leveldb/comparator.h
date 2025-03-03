#ifndef LEVELDB_INCLUDE_COMPARATOR_H_
#define LEVELDB_INCLUDE_COMPARATOR_H_

#include <string>
#include "export.h"

namespace leveldb {
    class Slice;
    class LEVELDB_EXPORT Comparator {
        public:
        virtual ~Comparator();
        virtual int Compare(const Slice& a, const Slice& b) const = 0;
        virtual const char* Name() const = 0;
        virtual void FindShortestSeparator(std::string* start,
                                            const Slice& limit) const = 0;
        virtual void FindShortSuccessor(std::string* key) const = 0;
    };
    LEVELDB_EXPORT const Comparator* BytewiseComparator();
}
#endif