// MemTable Key(<=>LookUpKey):| 键长度（Varint32） | InternalKey（用户键 + 序列号 + 类型） |
#include "memtable.h"
#include "dbformat.h"
#include "../include/leveldb/comparator.h"
#include "../include/leveldb/env.h"
#include "../include/leveldb/iterator.h"
#include "../util/coding.h"

namespace leveldb {
    /**
     * @brief 解析带32位变长整数长度前缀的Slice
     * 
     * @param data 
     * @return Slice 
     */
    static Slice GetLengthPrefixedSlice(const char* data) {
        uint32_t len;// 存储解析出的长度
        const char* p = data;// 当前指针位置
        p = GetVarint32Ptr(p, p+5, &len);// 解析变长32位整数，p 移动到实际数据起始位置  32位最多5字节
        return Slice(p, len);// 返回变长前缀后的数据部分
    }
    MemTable::MemTable(const InternalKeyComparator& comparator)
        : comparator_(comparator),
          refs_(0),
          table_(comparator_, &arena_)
        {}
    MemTable::~MemTable() {assert(refs_==0);}
    /**
     * @brief 估算内存使用量
     * 
     * @return size_t 
     */
    size_t MemTable::ApproximateMemoryUsage() {
        return arena_.MemoryUsage();// 直接返回Arena分配器的总内存使用量(Arena以块分配内存，此值包含可能未使用的预留空间,所以是估算)
    }
    /**
     * @brief 比较跳表节点中的键  
     * 运算符重载
     * @param aptr 
     * @param bptr 
     * @return int 
     */
    int MemTable::KeyComparator::operator()(const char* aptr, const char* bptr) const {
        Slice a = GetLengthPrefixedSlice(aptr);// 解析跳表节点中的键（带32位变长前缀）
        Slice b = GetLengthPrefixedSlice(bptr);
        return comparator.Compare(a, b);
    }
    /**
     * @brief 将用户键编码为跳表使用的格式：[Varint32长度][实际键数据]
     * 
     * @param scratch 临时字符串，用于存储编码结果
     * @param target 需要编码的用户键
     * @return const char* 编码后的字符指针
     */
    static const char* EncodeKey(std::string* scratch, const Slice& target) {
        scratch->clear();
        PutVarint32(scratch, target.size());// 写入变长长度
        scratch->append(target.data(), target.size());// 追加原始数据
        return scratch->data();
    }
    /**
     * @brief emTable迭代器实现
     * 其实就是调用SkipList的迭代器
     */
    class MemTableIterator : public Iterator {
        public:
            explicit MemTableIterator(MemTable::Table* table) 
                : iter_(table)// 初始化跳表迭代器
            {}
            // 禁止拷贝构造和赋值
            MemTableIterator(const MemTableIterator&) = delete;
            MemTableIterator& operator=(const MemTableIterator&) = delete;
            ~MemTableIterator() override = default;
            bool Valid() const override { return iter_.Valid(); }// 判断当前迭代器位置是否有效
            void Seek(const Slice& k) override { iter_.Seek(EncodeKey(&tmp_, k)); }// 定位到第一个大于等于k的键
            void SeekToFirst() override { iter_.SeekToFirst(); }// 定位到起始位置
            void SeekToLast() override { iter_.SeekToLast(); }// 定位到末尾位置
            // 移动迭代器
            void Next() override { iter_.Next(); }
            void Prev() override { iter_.Prev(); }
            Slice key() const override { return GetLengthPrefixedSlice(iter_.key()); }// 获取当前键（解析长度前缀后的用户键）
            // 获取当前值（键编码后的下一个长度前缀数据）
            Slice value() const override {
                Slice key_slice = GetLengthPrefixedSlice(iter_.key());
                return GetLengthPrefixedSlice(key_slice.data() + key_slice.size());// 值存储在键数据之后：key_data + key_size + Varint32_len + value
            }
            Status status() const override { return Status::OK(); }
        private:
            MemTable::Table::Iterator iter_;// 底层跳表迭代器
            std::string tmp_;  
    };
    /**
     * @brief 创建新的正向迭代器
     * 
     * @return Iterator* 
     */
    Iterator* MemTable::NewIterator() {
        return new MemTableIterator(&table_);
    }
    void MemTable::Add(SequenceNumber s, ValueType type, const Slice& key, const Slice& value) {
        size_t key_size = key.size();// 用户键原始长度
        size_t val_size = value.size();// 值原始长度
        size_t internal_key_size = key_size + 8;// InternalKey总长度（用户键 + 7字节序列号 + 1字节类型）
        // 计算编码后总长度：
        // [Varint32(internal_key_size)] + [InternalKey] + [Varint32(val_size)] + [Value]
        const size_t encoded_len = VarintLength(internal_key_size) + internal_key_size + VarintLength(val_size) + val_size;
        char* buf = arena_.Allocate(encoded_len);// 从内存池分配连续空间
        char* p = EncodeVarint32(buf, internal_key_size);// 编码 InternalKey 长度（Varint32格式）
        std::memcpy(p, key.data(), key_size);// 写入用户键原始数据
        p += key_size;
        // 编码序列号sequence_number和类型到8字节：
        // 高56位为序列号（s << 8），低8位为类型
        EncodeFixed64(p, (s << 8) | type);
        p += 8;
        p = EncodeVarint32(p, val_size);// 编码值长度（Varint32格式
        std::memcpy(p, value.data(), val_size);// 写入值原始数据
        assert(p + val_size == buf + encoded_len);// 验证内存计算正确性
        table_.Insert(buf);  // 将编码后的完整条目插入跳表
    }
    bool MemTable::Get(const LookupKey& key, std::string* value, Status* s) {
        Slice memkey = key.memtable_key();// 获取LookupKey的MemTable格式编码（含用户键+序列号+类型）
        // 创建跳表迭代器
        Table::Iterator iter(&table_);
        iter.Seek(memkey.data());// 定位到第一个>=目标键的位置
        if(iter.Valid()) {// 找到有效条目
            const char* entry = iter.key();// 获取跳表节点数据
            uint32_t key_length;// 解析InternalKey长度（Varint32解码）
            const char* key_ptr = GetVarint32Ptr(entry, entry+5, &key_length);
            // 使用用户自定义比较器比对键是否匹配
            if(comparator_.comparator.user_comparator()->Compare(Slice(key_ptr, key_length-8), key.user_key())==0) {
                const uint64_t tag = DecodeFixed64(key_ptr+key_length-8); // 解码最后8字节：高56位为序列号，低8位为类型
                switch(static_cast<ValueType>(tag&0xff)) {
                    case kTypeValue: {// 正常值
                        // 值存储在InternalKey之后
                        // [Varint32(val_size)][value_data]
                        Slice v = GetLengthPrefixedSlice(key_ptr+key_length);
                        value->assign(v.data(), v.size());
                        return true;
                    }
                    case kTypeDeletion:// 删除标记
                        *s = Status::NotFound(Slice());// 返回NotFound状态
                        return true;// 表示"找到"删除标记
                }
            }
        }
        return false;
    }

}