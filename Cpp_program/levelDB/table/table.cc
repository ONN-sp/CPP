#include "../includeleveldb/table.h"
#include "../includeleveldb/cache.h"
#include "../includeleveldb/comparator.h"
#include "../includeleveldb/env.h"
#include "../includeleveldb/filter_policy.h"
#include "../include/leveldb/options.h"
#include "block.h"
#include "filter_block.h"
#include "format.h"
#include "two_level_iterator.h"
#include "../util/coding.h"

namespace leveldb {
    struct Table::Rep {
        // 析构函数，释放相关资源
        ~Rep() {
            delete filter;// 删除过滤器
            delete[] filter_data;// 删除过滤器数据
            delete index_block;// 删除索引块
        }
        Options options;
        Status status;
        RandomAccessFile* file;
        uint64_t cache_id;// 缓存ID
        FilterBlockReader* filter;// 过滤器块读取器（如Bloom过滤器）
        const char* filter_data;// 过滤器块的原始数据指针
        BlockHandle metaindex_handle;// 元数据索引块的位置信息
        Block* index_block;// 已加载的索引块对象
    };
    // 打开表文件
    Status Table::Open(const Options& options, RandomAccessFile* file, uint64_t size, Table** table) {
        *table = nullptr;
        if (size < Footer::kEncodedLength)// 首先从文件的结尾读取Footer，并Decode到Footer对象中，如果文件长度小于Footer的长度，则报错
            return Status::Corruption("file is too short to be an sstable");
        // 读取Footer（最后48字节）
        char footer_space[Footer::kEncodedLength];
        Slice footer_input;
        Status s = file->Read(size - Footer::kEncodedLength, Footer::kEncodedLength, &footer_input, footer_space);
        if (!s.ok()) 
            return s;
        // 解码Footer内容
        Footer footer;
        s = footer.DecodeFrom(&footer_input);
        if (!s.ok()) 
            return s;
        // 读取data index block索引块（通过Footer中记录的位置）
        BlockContents index_block_contents;
        ReadOptions opt;
        if (options.paranoid_checks) 
            opt.verify_checksums = true;// 校验和验证
        s = ReadBlock(file, opt, footer.index_handle(), &index_block_contents);
        if (s.ok()) {// 读取meta index block
            Block* index_block = new Block(index_block_contents);// 构建索引块对象
            Rep* rep = new Table::Rep;
            rep->options = options;
            rep->file = file;
            rep->metaindex_handle = footer.metaindex_handle();
            rep->index_block = index_block;
            rep->cache_id = (options.block_cache ? options.block_cache->NewId() : 0);
            rep->filter_data = nullptr;
            rep->filter = nullptr;
            *table = new Table(rep);
            (*table)->ReadMeta(footer);// 加载元数据（如过滤器）
        }
        return s;
    }
    // 读取元数据信息
    void Table::ReadMeta(const Footer& footer) {
        if (rep_->options.filter_policy == nullptr)// 未启用过滤器 
            return;
        ReadOptions opt;
        if (rep_->options.paranoid_checks) 
            opt.verify_checksums = true;
        BlockContents contents;
        if (!ReadBlock(rep_->file, opt, footer.metaindex_handle(), &contents).ok())// 读取元数据索引块 
            return;
        Block* meta = new Block(contents);
        Iterator* iter = meta->NewIterator(BytewiseComparator());// 使用字节序比较器
        std::string key = "filter.";// 构建过滤器元数据键："filter.<PolicyName>"
        key.append(rep_->options.filter_policy->Name());
        iter->Seek(key);// 查找过滤器块的位置
        if (iter->Valid() && iter->key() == Slice(key)) 
            ReadFilter(iter->value());// 读取过滤器数据
        delete iter;
        delete meta;
    }
    // 加载过滤器块数据 
    void Table::ReadFilter(const Slice& filter_handle_value) {
        Slice v = filter_handle_value;
        BlockHandle filter_handle;
        if (!filter_handle.DecodeFrom(&v).ok())// 解码过滤器块的位置句柄
            return;
        ReadOptions opt;
        if (rep_->options.paranoid_checks)
            opt.verify_checksums = true;
        BlockContents block;
        if (!ReadBlock(rep_->file, opt, filter_handle, &block).ok())// 读取过滤器块内容
            return;
        if (block.heap_allocated)// 记录过滤器数据所有权和读取器 
            rep_->filter_data = block.data.data();// 标记堆内存需释放
        rep_->filter = new FilterBlockReader(rep_->options.filter_policy, block.data);
    }
    Table::~Table() { delete rep_; }
    // 直接删除块（无缓存时使用）
    static void DeleteBlock(void* arg, void* ignored) {
        delete reinterpret_cast<Block*>(arg);
    }
    // 缓存条目删除回调
    static void DeleteCachedBlock(const Slice& key, void* value) {
        Block* block = reinterpret_cast<Block*>(value);
        delete block;
    }
    // 释放缓存句柄（当迭代器不再需要缓存块时）
    static void ReleaseBlock(void* arg, void* h) {
        Cache* cache = reinterpret_cast<Cache*>(arg);
        Cache::Handle* handle = reinterpret_cast<Cache::Handle*>(h);
        cache->Release(handle);
    }
    /**
     * @brief 块读取器
     * 
     * @param arg 
     * @param options 
     * @param index_value 
     * @return Iterator* 
     */
    Iterator* Table::BlockReader(void* arg, const ReadOptions& options,
                                const Slice& index_value) {
        Table* table = reinterpret_cast<Table*>(arg);
        Cache* block_cache = table->rep_->options.block_cache;
        Block* block = nullptr;
        Cache::Handle* cache_handle = nullptr;
        BlockHandle handle;
        Slice input = index_value;
        Status s = handle.DecodeFrom(&input);
        if (s.ok()) {
            BlockContents contents;
            if (block_cache != nullptr) {// 启用块缓存
                // 生成缓存键：cache_id + 块偏移量（16字节）
                char cache_key_buffer[16];
                EncodeFixed64(cache_key_buffer, table->rep_->cache_id);
                EncodeFixed64(cache_key_buffer + 8, handle.offset());
                Slice key(cache_key_buffer, sizeof(cache_key_buffer));
                cache_handle = block_cache->Lookup(key);// 尝试从缓存获取
                if (cache_handle != nullptr)
                    block = reinterpret_cast<Block*>(block_cache->Value(cache_handle));
                else {// 缓存未命中，从磁盘读取
                    s = ReadBlock(table->rep_->file, options, handle, &contents);
                    if (s.ok()) {
                        block = new Block(contents);
                        if (contents.cachable && options.fill_cache)// 根据条件插入缓存 
                            cache_handle = block_cache->Insert(key, block, block->size(), &DeleteCachedBlock);
                    }
                }
            } 
            else {// 未启用缓存，直接读取
                s = ReadBlock(table->rep_->file, options, handle, &contents);
                if (s.ok())
                    block = new Block(contents);
            }
        }
        // 构建迭代器并注册清理函数
        Iterator* iter;
        if (block != nullptr) {
            iter = block->NewIterator(table->rep_->options.comparator);
            if (cache_handle == nullptr)
                iter->RegisterCleanup(&DeleteBlock, block, nullptr);// 无缓存：迭代器销毁时直接删除块
            else
                iter->RegisterCleanup(&ReleaseBlock, block_cache, cache_handle);// 有缓存：释放缓存句柄引用
        } 
        else 
            iter = NewErrorIterator(s);
        return iter;
    }
    // 迭代器创建
    Iterator* Table::NewIterator(const ReadOptions& options) const {
        // 创建两级迭代器：
        // 第1级：数据索引块迭代器
        // 第2级：通过BlockReader访问数据块
        return NewTwoLevelIterator(rep_->index_block->NewIterator(rep_->options.comparator),
            &Table::BlockReader, const_cast<Table*>(this), options);
    }
    // 内部查询实现
    Status Table::InternalGet(const ReadOptions& options, const Slice& k, void* arg,
                            void (*handle_result)(void*, const Slice&,
                                                    const Slice&)) {
        Status s;
        Iterator* iiter = rep_->index_block->NewIterator(rep_->options.comparator);// 创建索引块迭代器
        iiter->Seek(k);
        if (iiter->Valid()) {
            Slice handle_value = iiter->value();
            FilterBlockReader* filter = rep_->filter;
            BlockHandle handle;
            if (filter != nullptr && handle.DecodeFrom(&handle_value).ok() &&!filter->KeyMayMatch(handle.offset(), k))// 过滤器检查（快速排除）
            {
                // 键肯定不存在，跳过数据块读取
            } 
            else {// 读取数据块并查找键
                Iterator* block_iter = BlockReader(this, options, iiter->value());
                block_iter->Seek(k);
                if (block_iter->Valid())
                    (*handle_result)(arg, block_iter->key(), block_iter->value());// 调用回调处理结果
                s = block_iter->status();
                delete block_iter;
            }
        }
        if (s.ok())
            s = iiter->status();
        delete iiter;
        return s;
    }
    // 估算键的位置偏移
    uint64_t Table::ApproximateOffsetOf(const Slice& key) const {
        Iterator* index_iter = rep_->index_block->NewIterator(rep_->options.comparator);
        index_iter->Seek(key);
        uint64_t result;
        if (index_iter->Valid()) {
            BlockHandle handle;
            Slice input = index_iter->value();
            Status s = handle.DecodeFrom(&input);
            if (s.ok())
                result = handle.offset();// 返回数据块起始偏移
            else
                result = rep_->metaindex_handle.offset();// 错误时返回元数据索引位置
        } 
        else 
            result = rep_->metaindex_handle.offset();// 键大于所有索引，返回文件末尾
        delete index_iter;
        return result;
    }
}