#include "block.h"
#include <algorithm>
#include <cstdint>
#include <vector>
#include "../include/leveldb/comparator.h"
#include "format.h"
#include "../util/coding.h"
#include "../util/logging.h"

namespace leveldb {
    // 返回块中重启点的数量（存储在块结构的末尾的4字节）
    inline uint32_t Block::NumRestarts() const {
        assert(size_ >= sizeof(uint32_t));// 确保块大小至少包含重启点数量字段（4字节）
        return DecodeFixed32(data_ + size_ - sizeof(uint32_t)); // 解码块末尾的4字节（小端序）得到重启点数量
    }
    // Block构造函数：初始化块内容并验证格式有效性
    Block::Block(const BlockContents& contents)
        :   data_(contents.data.data()),// 原始数据指针
            size_(contents.data.size()),// 数据总大小
            owned_(contents.heap_allocated)// 是否拥有数据所有权 
    {
        if (size_ < sizeof(uint32_t))
            size_ = 0; // 块太小无法包含重启点数量字段，标记为无效
        else {
            // 计算最大允许的重启点数量 = (总大小 - 重启点数量字段) / 每个重启点4字节
            size_t max_restarts_allowed = (size_ - sizeof(uint32_t)) / sizeof(uint32_t);
            if (NumRestarts() > max_restarts_allowed)
                size_ = 0;// 重启点数量非法，标记为无效
            else 
                // 计算重启点数组的起始偏移 = 数据末尾 - (重启点数量 + 1)*4
                // +1 是包含存储重启点数量的字段本身
                restart_offset_ = size_ - (1 + NumRestarts()) * sizeof(uint32_t);
        }
    }
    // Block析构函数：释放堆分配的内存（如果拥有所有权）
    Block::~Block() {
        if (owned_)
            delete[] data_;// 仅当数据是堆分配时释放
    }
    // 解码键值对条目头信息（共享/非共享前缀长度、值长度）
    static inline const char* DecodeEntry(const char* p, const char* limit,
                                        uint32_t* shared, uint32_t* non_shared,
                                        uint32_t* value_length) {
        if (limit - p < 3)// 最少需要3字节存储头信息（当所有字段均<128时） 
            return nullptr;
        *shared = reinterpret_cast<const uint8_t*>(p)[0];
        *non_shared = reinterpret_cast<const uint8_t*>(p)[1];
        *value_length = reinterpret_cast<const uint8_t*>(p)[2];
        if ((*shared | *non_shared | *value_length) < 128) 
            p += 3;// 成功解码，指针前进3字节
        else {// 需要解码变长整数（Varint32）
            if ((p = GetVarint32Ptr(p, limit, shared)) == nullptr) return nullptr;
            if ((p = GetVarint32Ptr(p, limit, non_shared)) == nullptr) return nullptr;
            if ((p = GetVarint32Ptr(p, limit, value_length)) == nullptr) return nullptr;
        }
        if (static_cast<uint32_t>(limit - p) < (*non_shared + *value_length))// 检查键和值的总长度是否超出剩余空间 
            return nullptr;
        return p;// 返回键的非共享部分起始位置
    }
    /**
     * @brief 生成一个Block块的迭代器
     * 
     */
    class Block::Iter : public Iterator {
        private:
            const Comparator* const comparator_;// 键比较器
            const char* const data_;// 块数据起始指针     
            uint32_t const restarts_;// 重启点数组的起始偏移      
            uint32_t const num_restarts_;// 重启点总数 
            uint32_t current_;// 当前条目偏移
            uint32_t restart_index_;// 当前所在的重启点索引
            std::string key_;// 当前键（拼接后的完整键）
            Slice value_;// 当前值
            Status status_;// 迭代器状态（错误时设置）
            inline int Compare(const Slice& a, const Slice& b) const {// 键比较快捷方式
                return comparator_->Compare(a, b);
            }
            // 计算下一个条目(键值对)的起始偏移（当前值末尾位置）
            inline uint32_t NextEntryOffset() const {
                return (value_.data() + value_.size()) - data_;
            }
            // 获取指定索引的重启点偏移（从重启点数组读取）
            uint32_t GetRestartPoint(uint32_t index) {
                assert(index < num_restarts_);
                return DecodeFixed32(data_ + restarts_ + index * sizeof(uint32_t));// 每个重启点占4字节，小端序
            }
            // 定位到指定重启点，并初始化迭代状态
            void SeekToRestartPoint(uint32_t index) {
                key_.clear();// 清空当前键
                restart_index_ = index;// 设置当前重启点索引
                uint32_t offset = GetRestartPoint(index);// 获取重启点偏移
                value_ = Slice(data_ + offset, 0);// 设置值为空（等待ParseNextKey填充）
            }
        public:
            // 迭代器构造函数
            Iter(const Comparator* comparator, const char* data, uint32_t restarts,
                uint32_t num_restarts)
                : comparator_(comparator),
                    data_(data),
                    restarts_(restarts),// 重启点数组起始偏移
                    num_restarts_(num_restarts),// 重启点总数
                    current_(restarts_),// 初始位置设为重启点数组起始（无效位置）
                    restart_index_(num_restarts_)// 初始重启索引设为超出范围 
            {
                assert(num_restarts_ > 0); // 必须至少有一个重启点
            }
            bool Valid() const override { return current_ < restarts_; }// 有效条件：当前偏移在重启点数组之前
            Status status() const override { return status_; }
            Slice key() const override {
                assert(Valid());
                return key_;
            }
            Slice value() const override {
                assert(Valid());
                return value_;
            }
            // 移动到下一个键值对
            void Next() override {
                assert(Valid());
                ParseNextKey();// 直接解析下一个条目(键值对)
            }
            // 移动到前一个键值对
            void Prev() override {
                assert(Valid());
                const uint32_t original = current_;
                while (GetRestartPoint(restart_index_) >= original) {
                    if (restart_index_ == 0) {// 已到第一个重启点
                        current_ = restarts_;// 设为无效位置
                        restart_index_ = num_restarts_;
                        return;
                    }
                    restart_index_--;
                }
                SeekToRestartPoint(restart_index_);// 向前回跳到在current_前面的那个重启点，并定位到重启点的k/v对开始位置
                do {

                } while (ParseNextKey() && NextEntryOffset() < original);// 从重启点位置开始向后遍历，直到遇到original前面的那个k/v对。
            }
            /**
             * @brief 在Block块中查找指定键
             * 
             * @param target 
             */
            void Seek(const Slice& target) override {
                uint32_t left = 0;
                uint32_t right = num_restarts_ - 1;
                int current_key_compare = 0;
                if (Valid()) {// 若当前有效，利用当前位置优化查找范围
                    current_key_compare = Compare(key_, target);
                    if (current_key_compare < 0)
                        left = restart_index_;// 目标键在右侧重启区间
                    else if (current_key_compare > 0)
                        right = restart_index_;// 目标键在左侧重启区间
                    else
                        return;// 已命中，直接返回
                }
                // 二分查找重启点数组
                while (left < right) {
                    uint32_t mid = (left + right + 1) / 2;
                    uint32_t region_offset = GetRestartPoint(mid);
                    uint32_t shared, non_shared, value_length;
                    // 解码重启点mid对应的键（必须是完整键，shared应为0）
                    const char* key_ptr =
                        DecodeEntry(data_ + region_offset, data_ + restarts_, &shared,
                                    &non_shared, &value_length);
                    if (key_ptr == nullptr || (shared != 0)) {
                        CorruptionError();// 数据损坏：重启点条目必须存储完整键
                        return;
                    }
                    Slice mid_key(key_ptr, non_shared);
                    if (Compare(mid_key, target) < 0) 
                        left = mid;
                    else 
                        right = mid - 1;
                }
                assert(current_key_compare == 0 || Valid());
                bool skip_seek = left == restart_index_ && current_key_compare < 0;// 定位到候选重启点left
                if (!skip_seek)
                    SeekToRestartPoint(left);// 跳转到重启点left
                while (true) {// 线性扫描该区间内的条目
                    if (!ParseNextKey())// 解析失败或到末尾
                        return;
                    if (Compare(key_, target) >= 0)// 找到第一个 >= target 的键（命中或不存在）
                        return;
                }
            }
            // 定位到第一个键
            void SeekToFirst() override {
                SeekToRestartPoint(0);// 跳转到第一个重启点
                ParseNextKey();// 解析第一个条目
            }
            // 定位到最后一个键
            void SeekToLast() override {
                SeekToRestartPoint(num_restarts_ - 1);// 跳转到最后一个重启点
                while (ParseNextKey() && NextEntryOffset() < restarts_) {// 遍历直到块末尾（restarts_位置）
                
                }
            }
        private:
            // 处理数据损坏错误
            void CorruptionError() {
                current_ = restarts_;// 设为无效位置
                restart_index_ = num_restarts_;
                status_ = Status::Corruption("bad entry in block");
                key_.clear();// 清空当前键值
                value_.clear();
            }
            // 解析下一个键值对
            bool ParseNextKey() {
                current_ = NextEntryOffset();// 计算下一个条目起始偏移
                const char* p = data_ + current_;
                const char* limit = data_ + restarts_;// 重启点数组为结束位置(从块结构就能看出)
                if (p >= limit) {// 超出数据范围
                    current_ = restarts_;// 标记为无效
                    restart_index_ = num_restarts_;
                    return false;
                }
                uint32_t shared, non_shared, value_length;
                p = DecodeEntry(p, limit, &shared, &non_shared, &value_length);
                if (p == nullptr || key_.size() < shared) {// 解码失败 或 共享长度超过当前键长度（数据损坏）
                    CorruptionError();
                    return false;
                } 
                else {// 构建完整键：共享部分 + 非共享部分
                    key_.resize(shared);// 保留共享前缀
                    key_.append(p, non_shared);// 追加非共享部分
                    value_ = Slice(p + non_shared, value_length);// 设置值
                    while (restart_index_ + 1 < num_restarts_ && GetRestartPoint(restart_index_ + 1) < current_)// 更新当前重启点索引（找到包含当前条目的重启区间）
                        ++restart_index_;
                    return true;
                }
            }
    };
    // 创建块迭代器
    Iterator* Block::NewIterator(const Comparator* comparator) {
        if (size_ < sizeof(uint32_t))// 块格式无效
            return NewErrorIterator(Status::Corruption("bad block contents"));
        const uint32_t num_restarts = NumRestarts();
        if (num_restarts == 0)// 无数据（只有重启点数组）
            return NewEmptyIterator();
        else 
            return new Iter(comparator, data_, restart_offset_, num_restarts);
    }
}