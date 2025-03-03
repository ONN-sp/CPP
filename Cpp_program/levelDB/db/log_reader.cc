#include "log_reader.h"
#include <cstdio>
#include "../include/leveldb/env.h"
#include "../util/coding.h"
#include "../util/crc32c.h"

namespace leveldb {
    namespace log {
        Reader::Reporter::~Reporter() = default;
        Reader::Reader(SequentialFile* file, Reporter* reporter, bool checksum, uint64_t initial_offset)
          : file_(file),
            reporter_(reporter),
            checksum_(checksum),
            backing_store_(new char[kBlockSize]),
            buffer_(),
            eof_(false),
            last_record_offset_(0),
            end_of_buffer_offset_(0),
            initial_offset_(initial_offset),
            resyncing_(initial_offset > 0) 
        {}
        Reader::~Reader() { delete[] backing_store_; }
        /**
         * @brief 跳过到初始偏移量所在的块起始位置
         * 
         * @return true 
         * @return false 
         */
        bool Reader::SkipToInitialBlock() {
            const size_t offset_in_block = initial_offset_ % kBlockSize;// 计算在块内的偏移量
            uint64_t block_start_location = initial_offset_ - offset_in_block; // 计算块起始位置
            // 处理特殊情况：当偏移量接近块末尾时(剩余空间不足6字节)
            // 因为记录头需要7字节(长度2+类型1+校验和4)，但这里判断是>kBlockSize-6
            if (offset_in_block > kBlockSize - 6) 
                block_start_location += kBlockSize;// 跳到下一个块
            end_of_buffer_offset_ = block_start_location;
            if (block_start_location > 0) {// 跳过文件开头到块起始位置
                Status skip_status = file_->Skip(block_start_location);
                if (!skip_status.ok()) {
                    ReportDrop(block_start_location, skip_status);
                    return false;
                }
            }
            return true;
        }
        /**
         * @brief 读取下一条完整记录到*record
         * 
         * @param record 
         * @param scratch 
         * @return true 
         * @return false 
         */
        bool Reader::ReadRecord(Slice* record, std::string* scratch) {
            if (last_record_offset_ < initial_offset_) {// 如果上次读取位置小于初始偏移量(首次读取或需要跳过)
                if (!SkipToInitialBlock())
                    return false;
            }
            scratch->clear();// 清空暂存区(用于拼接分块记录)
            record->clear(); // 清空输出记录
            bool in_fragmented_record = false;// 是否在分块记录中
            uint64_t prospective_record_offset = 0;// 当前处理记录的起始偏移量
            Slice fragment;// 当前物理记录片段
            while (true) {
                const unsigned int record_type = ReadPhysicalRecord(&fragment);// 读取记录保存到fragment
                uint64_t physical_record_offset = end_of_buffer_offset_ - buffer_.size() - kHeaderSize - fragment.size();// 计算当前物理记录在文件中的起始偏移量
                if (resyncing_) {// 处理重新同步状态
                    if (record_type == kMiddleType)
                        continue;// 跳过中间类型，直到找到起始或完整记录
                    else if (record_type == kLastType) {
                        resyncing_ = false;// 遇到结束片段后停止跳过
                        continue;
                    } 
                    else 
                        resyncing_ = false;// 遇到其他类型则退出同步状态
                }
                // 根据记录类型判断是否需要将当前读取的记录附加到scratch并记录读取
                switch (record_type) {
                    case kFullType:// 完整记录
                        if (in_fragmented_record) {
                            if (!scratch->empty())
                                ReportCorruption(scratch->size(), "partial record without end(1)");
                        }
                        prospective_record_offset = physical_record_offset;
                        scratch->clear();
                        *record = fragment;
                        last_record_offset_ = prospective_record_offset;
                        return true;
                    case kFirstType:// 分块记录的起始
                        if (in_fragmented_record) {
                            if (!scratch->empty())
                                ReportCorruption(scratch->size(), "partial record without end(2)");
                        }
                        prospective_record_offset = physical_record_offset;
                        scratch->assign(fragment.data(), fragment.size());
                        in_fragmented_record = true;
                        break;
                    case kMiddleType:// 分块记录的中间部分
                        if (!in_fragmented_record)
                            ReportCorruption(fragment.size(), "missing start of fragmented record(1)");
                        else
                            scratch->append(fragment.data(), fragment.size());
                        break;
                    case kLastType:// 分块记录的结束
                        if (!in_fragmented_record) 
                            ReportCorruption(fragment.size(), "missing start of fragmented record(2)");
                        else {
                            scratch->append(fragment.data(), fragment.size());
                            *record = Slice(*scratch);
                            last_record_offset_ = prospective_record_offset;
                            return true;
                        }
                        break;
                    case kEof:// 文件结束
                        if (in_fragmented_record) 
                            scratch->clear();
                        return false;
                    case kBadRecord:// 损坏的记录
                        if (in_fragmented_record) {
                            ReportCorruption(scratch->size(), "error in middle of record");
                            in_fragmented_record = false;
                            scratch->clear();
                        }
                        break;
                    default: {// 未知记录类型
                        char buf[40];
                        std::snprintf(buf, sizeof(buf), "unknown record type %u", record_type);
                        ReportCorruption((fragment.size() + (in_fragmented_record ? scratch->size() : 0)), buf);
                        in_fragmented_record = false;
                        scratch->clear();
                        break;
                    }
                }
            }
            return false;
        }
        uint64_t Reader::LastRecordOffset() { return last_record_offset_; }// 获取最后成功读取记录的偏移量
         // 报告数据损坏
        void Reader::ReportCorruption(uint64_t bytes, const char* reason) {
            ReportDrop(bytes, Status::Corruption(reason));
        }
        // 报告数据丢失/损坏
        void Reader::ReportDrop(uint64_t bytes, const Status& reason) {
            if (reporter_ != nullptr && end_of_buffer_offset_ - buffer_.size() - bytes >= initial_offset_)
                reporter_->Corruption(static_cast<size_t>(bytes), reason);
        }
        // 读取单个物理记录(可能跨多个块)
        unsigned int Reader::ReadPhysicalRecord(Slice* result) {
            while (true) {
                if (buffer_.size() < kHeaderSize) {// 检查缓冲区是否足够读取记录头(7字节)
                    if (!eof_) {
                        // 从文件读取新块
                        buffer_.clear();
                        Status status = file_->Read(kBlockSize, &buffer_, backing_store_);
                        end_of_buffer_offset_ += buffer_.size();// 更新缓冲区结束位置
                        if (!status.ok()) {// 读取失败
                            buffer_.clear();
                            ReportDrop(kBlockSize, status);
                            eof_ = true;
                            return kEof;
                        } 
                        else if (buffer_.size() < kBlockSize) 
                            eof_ = true;// 文件实际读取不足块大小，到达文件末尾
                        continue;// 继续循环以处理新数据
                    } 
                    else {
                        buffer_.clear();
                        return kEof;
                    }
                }
                const char* header = buffer_.data();// 解析记录头
                // 解析长度字段(小端存储)
                const uint32_t a = static_cast<uint32_t>(header[4]) & 0xff;
                const uint32_t b = static_cast<uint32_t>(header[5]) & 0xff;
                const unsigned int type = header[6];
                const uint32_t length = a | (b << 8);
                // 检查长度是否有效
                if (kHeaderSize + length > buffer_.size()) {
                    size_t drop_size = buffer_.size();
                    buffer_.clear();// 丢弃当前无效数据
                    if (!eof_) {
                        ReportCorruption(drop_size, "bad record length");
                        return kBadRecord;
                    }
                    return kEof;// 文件末尾允许不完整记录
                }
                if (type == kZeroType && length == 0) {
                    buffer_.clear();
                    return kBadRecord;
                }
                if (checksum_) {// 校验和验证
                    // 计算实际校验和(类型字段+数据)
                    uint32_t expected_crc = crc32c::Unmask(DecodeFixed32(header));
                    uint32_t actual_crc = crc32c::Value(header + 6, 1 + length);
                    if (actual_crc != expected_crc) {
                        size_t drop_size = buffer_.size();
                        buffer_.clear();
                        ReportCorruption(drop_size, "checksum mismatch");
                        return kBadRecord;
                    }
                }
                buffer_.remove_prefix(kHeaderSize + length);// 移除已处理的头和数据
                if (end_of_buffer_offset_ - buffer_.size() - kHeaderSize - length < initial_offset_) {// 检查记录是否在初始偏移量之前(需要跳过)
                    result->clear();
                    return kBadRecord;
                }
                *result = Slice(header + kHeaderSize, length);// 返回数据片段
                return type;
            }
        }
    }
}