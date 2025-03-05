#include "../incldue/leveldb/table_builder.h"
#include <cassert>
#include "../incldue/leveldb/comparator.h"
#include "../incldue/leveldb/env.h"
#include "../incldue/leveldb/filter_policy.h"
#include "../incldue/leveldb/options.h"
#include "block_builder.h"
#include "filter_block.h"
#include "format.h"
#include "../util/coding.h"
#include "../util/crc32c.h"

namespace leveldb {
    /**
     * @brief Rep结构体的成员就是TableBuilder所有的成员变量
     * 
     */
    struct TableBuilder::Rep {
        Rep(const Options& opt, WritableFile* f)
            : options(opt),
                index_block_options(opt),
                file(f),
                offset(0),
                data_block(&options),
                index_block(&index_block_options),
                num_entries(0),
                closed(false),
                filter_block(opt.filter_policy == nullptr
                                ? nullptr
                                : new FilterBlockBuilder(opt.filter_policy)),
                pending_index_entry(false) 
        {
            index_block_options.block_restart_interval = 1;
        }
        Options options;// data block的选项
        Options index_block_options;// index block的选项
        WritableFile* file;// sstable文件
        uint64_t offset;// 要写入data block在sstable文件中的偏移，初始0
        Status status;//当前状态-初始ok
        BlockBuilder data_block;//当前操作的data block
        BlockBuilder index_block;// sstable的index block
        std::string last_key;//当前data block最后的k/v对的key
        int64_t num_entries;// 当前data block的个数，初始0
        bool closed;//调用了Finish() or Abandon()，初始false 
        FilterBlockBuilder* filter_block;
        // 判断是否需要生成SST中的数据索引,SST中每次生成一个完整的块后,需要将该值置为true,说明需要为该块添加索引
        bool pending_index_entry;
        BlockHandle pending_handle;// 记录需要生成数据索引的数据块在SST中的偏移量和大小  
        std::string compressed_output;// 压缩后的data block，临时存储，写入后即被清空
    };
    // 构造函数：初始化TableBuilder
    TableBuilder::TableBuilder(const Options& options, WritableFile* file)
        : rep_(new Rep(options, file)) 
    {
        if (rep_->filter_block != nullptr) // 如果启用过滤器，初始化第一个过滤器块
            rep_->filter_block->StartBlock(0);
    }
    // 析构函数：确保资源释放（必须已调用Finish()或Abandon()）
    TableBuilder::~TableBuilder() {
        assert(rep_->closed); 
        delete rep_->filter_block;
        delete rep_;
    }
    // 动态修改配置选项（有限制条件）
    Status TableBuilder::ChangeOptions(const Options& options) {
        if (options.comparator != rep_->options.comparator)// 禁止修改比较器（会影响键顺序） 
            return Status::InvalidArgument("changing comparator while building table");
        rep_->options = options;
        rep_->index_block_options = options;
        rep_->index_block_options.block_restart_interval = 1;// 保持索引块配置
        return Status::OK();
    }
    /**
     * @brief 添加键值对到当前数据块
     * 
     * @param key 
     * @param value 
     */
    void TableBuilder::Add(const Slice& key, const Slice& value) {
        Rep* r = rep_;
        assert(!r->closed);
        if (!ok()) return;
        if (r->num_entries > 0)// 键顺序验证（首个键跳过）
            assert(r->options.comparator->Compare(key, Slice(r->last_key)) > 0);
        if (r->pending_index_entry) {// 判断是否需要增加数据索引.当完整生成一个块后,需要写入数据索引
            assert(r->data_block.empty());
            r->options.comparator->FindShortestSeparator(&r->last_key, key);// 查找该块中的最大键与即将插入键(即下一个块的最小键)之间的最短分隔符
            std::string handle_encoding;
            r->pending_handle.EncodeTo(&handle_encoding);// 将该块的BlockHandle编码,即将偏移量和大小分别编码为可变长度的64位整型
            r->index_block.Add(r->last_key, Slice(handle_encoding));// index block以上一个块的最后一个键值为索引块的键插入
            r->pending_index_entry = false;// 置为false,等待下一次生成一个完整的block并将该值再次置为true
        }
        if (r->filter_block != nullptr)// 在元数据块中增加该key 
            r->filter_block->AddKey(key);
        r->last_key.assign(key.data(), key.size());// 将last_key赋值为即将插入的键
        r->num_entries++;
        r->data_block.Add(key, value);// 在数据块中增加该键值对
        const size_t estimated_block_size = r->data_block.CurrentSizeEstimate();
        if (estimated_block_size >= r->options.block_size)// 检查数据块是否达到阈值(默认为4KB)，需要刷盘 
            Flush();
    }
    void TableBuilder::Flush() {
        Rep* r = rep_;
        assert(!r->closed);// 首先保证未关闭，且状态ok
        if (!ok()) 
            return;
        if (r->data_block.empty())// data block是空的 
            return;
        assert(!r->pending_index_entry);// 保证pending_index_entry为false，即data block的Add已经完成
        WriteBlock(&r->data_block, &r->pending_handle);// 写入data block，并设置其index entry信息—BlockHandle对象
        // 写入成功，则Flush文件，并设置r->pending_index_entry为true，
        // 以根据下一个data block的first key调整index entry的key—即r->last_key
        if (ok()) {
            r->pending_index_entry = true;
            r->status = r->file->Flush(); // 确保数据落盘
        }
        if (r->filter_block != nullptr)// 通知过滤器块新数据块开始（基于新偏移量） 
            r->filter_block->StartBlock(r->offset);
    }
    // 将构建好的块写入文件（含压缩处理）
    void TableBuilder::WriteBlock(BlockBuilder* block, BlockHandle* handle) {
        assert(ok());
        Rep* r = rep_;
        Slice raw = block->Finish();// 获取未压缩数据
        Slice block_contents;
        CompressionType type = r->options.compression;
        switch (type) {
            case kNoCompression:
                block_contents = raw;
                break;
            case kSnappyCompression: 
            {
                std::string* compressed = &r->compressed_output;
                if (port::Snappy_Compress(raw.data(), raw.size(), compressed) && compressed->size() < raw.size() - (raw.size() / 8u))// Snappy压缩且压缩率至少优于12.5%才使用
                    block_contents = *compressed; 
                else {// 压缩率太低就还是作为未压缩的内容存储
                    block_contents = raw;
                    type = kNoCompression;
                }
                break;
            }
            case kZstdCompression: 
            {
                std::string* compressed = &r->compressed_output;
                if (port::Zstd_Compress(r->options.zstd_compression_level, raw.data(), raw.size(), compressed) && compressed->size() < raw.size() - (raw.size() / 8u))// Zstd压缩且压缩率优于12.5%才使用
                    block_contents = *compressed;
                else {
                    block_contents = raw;
                    type = kNoCompression;
                }
                break;
            }
        }
        WriteRawBlock(block_contents, type, handle);// 写入最终块内容（可能压缩后的）
        r->compressed_output.clear();
        block->Reset();// 重置块构建器
    }
    // 实际文件写入操作（含类型和校验和）
    void TableBuilder::WriteRawBlock(const Slice& block_contents, CompressionType type, BlockHandle* handle) {
        Rep* r = rep_;
        handle->set_offset(r->offset);
        handle->set_size(block_contents.size());// 为index设置data block的handle信息
        r->status = r->file->Append(block_contents);// 写入data block内容
        if (r->status.ok()) {
            // 写入1byte的type和4bytes的crc32
            char trailer[kBlockTrailerSize];
            trailer[0] = type;
            uint32_t crc = crc32c::Value(block_contents.data(), block_contents.size());
            crc = crc32c::Extend(crc, trailer, 1);  // Extend crc to cover block type
            EncodeFixed32(trailer + 1, crc32c::Mask(crc));
            r->status = r->file->Append(Slice(trailer, kBlockTrailerSize));
            if (r->status.ok())// 写入成功更新offset-下一个data block的写入偏移 
                r->offset += block_contents.size() + kBlockTrailerSize;
        }
    }
    Status TableBuilder::status() const { return rep_->status; }
    /**
     * @brief // 完成SSTable构建
     * 
     * @return Status 
     */
    Status TableBuilder::Finish() {
        Rep* r = rep_;
        Flush();// 将数据块写入SST文件并且刷盘
        assert(!r->closed);
        r->closed = true;
        BlockHandle filter_block_handle, metaindex_block_handle, index_block_handle;
        if (ok() && r->filter_block != nullptr)
            WriteRawBlock(r->filter_block->Finish(), kNoCompression, &filter_block_handle);// 写入元数据块
        if (ok()) {// 构建并写入元数据索引块
            BlockBuilder meta_index_block(&r->options);
            if (r->filter_block != nullptr) {
            // 添加过滤器块的元数据条目
            std::string key = "filter.";
            key.append(r->options.filter_policy->Name());
            std::string handle_encoding;
            filter_block_handle.EncodeTo(&handle_encoding);
            meta_index_block.Add(key, handle_encoding);
            }
            WriteBlock(&meta_index_block, &metaindex_block_handle);
        }
        if (ok()) {// 写入数据索引块
            if (r->pending_index_entry) {
                r->options.comparator->FindShortSuccessor(&r->last_key);
                std::string handle_encoding;
                r->pending_handle.EncodeTo(&handle_encoding);
                r->index_block.Add(r->last_key, Slice(handle_encoding));
                r->pending_index_entry = false;
            }
            WriteBlock(&r->index_block, &index_block_handle);
        }
        if (ok()) {// 写入尾部48字节
            Footer footer;
            footer.set_metaindex_handle(metaindex_block_handle);
            footer.set_index_handle(index_block_handle);
            std::string footer_encoding;
            footer.EncodeTo(&footer_encoding);
            r->status = r->file->Append(footer_encoding);
            if (r->status.ok()) 
            r->offset += footer_encoding.size();
        }
        return r->status;
    }
    // 结束表的构建，并丢弃当前缓存的内容，该方法被调用后，将不再会使用传入的WritableFile
    void TableBuilder::Abandon() {
        Rep* r = rep_;
        assert(!r->closed);

        r->closed = true;
    }
    uint64_t TableBuilder::NumEntries() const { return rep_->num_entries; }// 已添加的键值对总数
    uint64_t TableBuilder::FileSize() const { return rep_->offset; }// 当前文件总大小（含未刷新的数据）
}