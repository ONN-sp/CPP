#ifndef LEVELDB_DB_MEMTABLE_H_
#define LEVELDB_DB_MEMTABLE_H_

#include <string>

#include "dbformat.h"
#include "skiplist.h"
#include "../include/leveldb/db.h"
#include "../util/arena.h"

namespace leveldb {
    class InternalKeyComparator;
    class MemTableIterator;
    /**
     * @brief 用于在内存中存储数据，基于跳表（SkipList）实现，支持快速插入和查找
     * 
     */
    class MemTable {
    public:
    explicit MemTable(const InternalKeyComparator& comparator);// 显式构造函数，接收一个 InternalKeyComparator 用于键的比较
    MemTable(const MemTable&) = delete;
    MemTable& operator=(const MemTable&) = delete;
    void Ref() { ++refs_; }// 增加引用计数
    void Unref() {// 减少引用计数，当计数为0时自动销毁对象
        --refs_;
        assert(refs_ >= 0);
        if (refs_ <= 0)
            delete this;
    }
    size_t ApproximateMemoryUsage();// 估算 MemTable 占用的近似内存大小（包括键值对和跳表结构）
    Iterator* NewIterator();// 创建一个新的迭代器，用于遍历 MemTable 中的全部键值对
    void Add(SequenceNumber seq, ValueType type, const Slice& key, const Slice& value);// 向 MemTable 中添加一个键值对
    bool Get(const LookupKey& key, std::string* value, Status* s);// 根据给定的查找键（LookupKey）检索对应的值
    private:
    friend class MemTableIterator;// 友元类声明，允许迭代器类访问MemTable类的私有成员
    friend class MemTableBackwardIterator;
    struct KeyComparator {// 内部键比较器结构体，用于适配跳表所需的比较接口
        const InternalKeyComparator comparator;// 实际的内部键比较器
        explicit KeyComparator(const InternalKeyComparator& c) : comparator(c) {}
        int operator()(const char* a, const char* b) const;
    };
    typedef SkipList<const char*, KeyComparator> Table;// 定义跳表类型，节点键为 const char*，使用 KeyComparator 进行比较
    ~MemTable();
    KeyComparator comparator_;// 键比较器
    int refs_;// 引用计数器，初始为 1
    Arena arena_;// 内存分配器，用于高效分配键值对内存
    Table table_;// 跳表实例，存储实际数据
    };
}
#endif