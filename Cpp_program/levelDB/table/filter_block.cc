#include "filter_block.h"
#include "../include/leveldb/filter_policy.h"
#include "../util/coding.h"

namespace leveldb {
    static const size_t kFilterBaseLg = 11;
    static const size_t kFilterBase = 1 << kFilterBaseLg;
    FilterBlockBuilder::FilterBlockBuilder(const FilterPolicy* policy)
        : policy_(policy) 
    {}
    void FilterBlockBuilder::StartBlock(uint64_t block_offset) {
        uint64_t filter_index = (block_offset / kFilterBase);
        assert(filter_index >= filter_offsets_.size());
        while (filter_index > filter_offsets_.size()) 
            GenerateFilter();
    }
    /**
     * @brief 将每个键放置到keys_中,并将键的索引位置记录到start_
     * 
     * @param key 
     */
    void FilterBlockBuilder::AddKey(const Slice& key) {
        Slice k = key;
        start_.push_back(keys_.size());
        keys_.append(k.data(), k.size());// 将键key追加到keys_中
    }
    /**
     * @brief 生成一个元数据块
     * 
     * @return Slice 
     */
    Slice FilterBlockBuilder::Finish() {
        if (!start_.empty()) 
            GenerateFilter();
        const uint32_t array_offset = result_.size();
        for (size_t i = 0; i < filter_offsets_.size(); i++)// 将过滤器偏移量依次追加到result_中,每个偏移量占4字节
            PutFixed32(&result_, filter_offsets_[i]);
        PutFixed32(&result_, array_offset);
        result_.push_back(kFilterBaseLg);// 将过滤器基数追加到result_  
        return Slice(result_);
    }
    /**
     * @brief 生成布隆过滤器内容
     * 
     */
    void FilterBlockBuilder::GenerateFilter() {
        const size_t num_keys = start_.size();
        if (num_keys == 0) {
            filter_offsets_.push_back(result_.size());
            return;
        }
        start_.push_back(keys_.size()); 
        tmp_keys_.resize(num_keys);
        for (size_t i = 0; i < num_keys; i++) {
            const char* base = keys_.data() + start_[i];
            size_t length = start_[i + 1] - start_[i];
            tmp_keys_[i] = Slice(base, length);
        }
        filter_offsets_.push_back(result_.size());
        policy_->CreateFilter(&tmp_keys_[0], static_cast<int>(num_keys), &result_);
        tmp_keys_.clear();
        keys_.clear();
        start_.clear();
    }
    /**
     * @brief FilterBlockReader类用于查找一个元素是否在一个布隆过滤器中
     * 
     * @param policy 
     * @param contents 
     */
    FilterBlockReader::FilterBlockReader(const FilterPolicy* policy, const Slice& contents)
        : policy_(policy), data_(nullptr), offset_(nullptr), num_(0), base_lg_(0) 
    {
        size_t n = contents.size();
        if (n < 5)
            return; 
        base_lg_ = contents[n - 1];// 读取最后1字节的过滤器基数放到成员变量base_lg_
        uint32_t last_word = DecodeFixed32(contents.data() + n - 5);// 将过滤器内容总大小的值放到last_word中
        if (last_word > n - 5) 
            return;
        data_ = contents.data();// data_指向元数据块的开始位置
        offset_ = data_ + last_word;// offset_为过滤器偏移量的开始位置
        num_ = (n - 5 - last_word) / 4;// num_为过滤器偏移量个数
    }
    // 判断给定键key是否在布隆过滤器中
    bool FilterBlockReader::KeyMayMatch(uint64_t block_offset, const Slice& key) {
        uint64_t index = block_offset >> base_lg_;
        if (index < num_) {
            uint32_t start = DecodeFixed32(offset_ + index * 4);//
            uint32_t limit = DecodeFixed32(offset_ + index * 4 + 4);
            if (start <= limit && limit <= static_cast<size_t>(offset_ - data_)) {
                Slice filter = Slice(data_ + start, limit - start);
                return policy_->KeyMayMatch(key, filter);
            } 
            else if (start == limit) 
                return false;
        }
        return true; 
    }
}