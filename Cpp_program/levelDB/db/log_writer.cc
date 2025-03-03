#include "log_writer.h"
#include <cstdint>
#include "../include/leveldb/env.h"
#include "../util/coding.h"
#include "../util/crc32c.h"

namespace leveldb {
    namespace log {
        // 初始化记录类型对应的预计算CRC值数组
        static void InitTypeCrc(uint32_t* type_crc) {
            for (int i = 0; i <= kMaxRecordType; i++) {// 遍历所有可能的记录类型（0-4）
                char t = static_cast<char>(i);
                type_crc[i] = crc32c::Value(&t, 1);// 计算单个字符的CRC值（记录类型CRC预计算）
            }
        }
        // Writer构造函数（适用于新建Log文件）
        Writer::Writer(WritableFile* dest) : dest_(dest), block_offset_(0) {
            InitTypeCrc(type_crc_);
        }
        // Writer构造函数（适用于已有Log文件追加）
        Writer::Writer(WritableFile* dest, uint64_t dest_length)
            : dest_(dest), block_offset_(dest_length % kBlockSize) 
        {
            InitTypeCrc(type_crc_);
        }
        Writer::~Writer() = default;
        /**
         * @brief 添加一条逻辑记录到日志
         * 
         * @param slice 
         * @return Status 
         */
        Status Writer::AddRecord(const Slice& slice) {
            const char* ptr = slice.data();// ptr指向需要写入的记录内容
            size_t left = slice.size();// left表示需要写入的记录内容长度
            Status s;
            bool begin = true;// begin=true表明该条记录是第一次写入,即如果一个记录跨越多个块,只有写入第一个块时begin才为true,其它时候是false,通过该值可以确定头部类型字段是否为kFirstType
            do {
                // kBlockSize为一个块的大小(32768字节),block_offset_代表当前块的写入偏移量,因此leftover表明当前块还剩余多少字节可用
                const int leftover = kBlockSize - block_offset_;
                assert(leftover >= 0);
                if (leftover < kHeaderSize) {// 处理块剩余空间不足的情况（不足7字节头空间） 需要填充\x00
                    if (leftover > 0) {
                        static_assert(kHeaderSize == 7, "");
                        dest_->Append(Slice("\x00\x00\x00\x00\x00\x00", leftover));
                    }
                    block_offset_ = 0;
                }
                assert(kBlockSize - block_offset_ - kHeaderSize >= 0);
                const size_t avail = kBlockSize - block_offset_ - kHeaderSize;// 计算当前可用空间（总块大小 - 当前偏移 - 头大小）
                const size_t fragment_length = (left < avail) ? left : avail;// 当前块能写入的数据大小取决于记录剩余内容和块剩余空间之中比较小的值
                RecordType type;
                const bool end = (left == fragment_length);// end表示该条记录是否已经完整写入
                if (begin && end)// 通过begin和end确定记录的头部类型信息
                    type = kFullType;
                else if (begin) 
                    type = kFirstType;
                else if (end)
                    type = kLastType;
                else
                    type = kMiddleType;
                s = EmitPhysicalRecord(type, ptr, fragment_length);// 按Log文件的一条记录格式将数据写入并刷新到磁盘(头部信息+内容信息)
                ptr += fragment_length;// 移动数据指针
                left -= fragment_length;// 减少剩余数据量
                begin = false;// 后续分块不再是开始分块
            } while (s.ok() && left > 0);// 循环直到数据写完或出错
            return s;
        }
        /**
         * @brief 写入单个物理记录分块(即会Flush到磁盘)
         * 
         * @param t 
         * @param ptr 
         * @param length 
         * @return Status 
         */
        Status Writer::EmitPhysicalRecord(RecordType t, const char* ptr, size_t length) {
            assert(length <= 0xffff);
            assert(block_offset_ + kHeaderSize + length <= kBlockSize);
            char buf[kHeaderSize];// 7字节头缓冲区
            // 设置长度字段（小端存储
            buf[4] = static_cast<char>(length & 0xff);
            buf[5] = static_cast<char>(length >> 8);
            buf[6] = static_cast<char>(t);
            // 计算CRC校验和（使用预计算的类型CRC值）
            uint32_t crc = crc32c::Extend(type_crc_[t], ptr, length);
            crc = crc32c::Mask(crc);
            EncodeFixed32(buf, crc);
            // 先写入记录头
            Status s = dest_->Append(Slice(buf, kHeaderSize));
            if (s.ok()) {
                // 再写入实际数据
                s = dest_->Append(Slice(ptr, length));
                if (s.ok()) 
                    s = dest_->Flush();// 确保数据刷入持久存储
            }
            block_offset_ += kHeaderSize + length;// 更新块内偏移量
            return s;
        }
    }
}