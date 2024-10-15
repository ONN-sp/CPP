#ifndef RAPIDJSON_MEMORYSTREAM_H_
#define RAPIDJSON_MEMORYSTREAM_H_

#include "stream.h"

namespace RAPIDJSON{
    /**
     * @brief 定义一个内存字节流.用于处理内存中的输入字节流,与文件流不同,它操作的是内存缓冲区memorybuffer
     * MemoryStream()用于处理输入字节流,即提供了一系列的接口,如:Peek()、Take()、Tell()、Peek4()
     * Put()、Flush()、PutEnd()是给流提供写入功能的,memorystream不支持
     * 
     * 此类主要用与EncodedInputStream或AutoUTFInputStream类结合使用(但本项目仅限于UTF-8)
     */
    struct MemoryStream{
        MemoryStream(const char* src, size_t size)
            : src_(src), begin_(src), end_(src+size), size_(size)
        {}
        /**
         * @brief 用于查看当前字节但不移动读取到的字节指针
         * 
         * @return 返回当前字节地址;若到了缓冲区末尾就返回'\0'
         */
        char Peek() const { return RAPIDJSON_UNLIKELY(src_ == end_) ? '\0':*src_;}
        /**
         * @brief 用于读取当前字节并将读取到的字节指针移动到下一个字节
         * 
         * @return 返回当前字节,若已到缓冲区末尾则返回'\0' 
         */
        char Take() {return RAPIDJSON_UNLIKELY(src_==end_) ? '\0':*src_++;}
        /**
         * @brief 返回当前读取位置相对于起始位置的偏移量
         * 
         * @return 返回偏移量
         */
        size_t Tell() const {return static_cast<size_t>(src_-begin_);}
        /**
         * @brief PutBegin() 不支持写入操作,因此在调用时断言失败
         * 
         * @return char* 
         */
        char* PutBegin() {RAPIDJSON_ASSERT(false); return 0;}
        /**
         * @brief Put() 不支持写入操作,因此在调用时断言失败
         * 
         */
        void Put(char) {RAPIDJSON_ASSERT(false);}
        /**
         * @brief Flush() 不支持写入操作,因此在调用时断言失败
         * 
         */
        void Flush() {RAPIDJSON_ASSERT(false);}
        /**
         * @brief PutEnd() 不支持写入操作,因此在调用时断言失败
         * 
         * @return size_t 
         */
        size_t PutEnd(char*) {RAPIDJSON_ASSERT(false); return 0;}
        /**
         * @brief Peek4()用于编码检测,查看接下来的4个字节
         * 
         * @return 如果剩余字节>=4个,则返回当前读取位置;否则返回0
         */
        const char* Peek4() const {
            return Tell()+4 <= size_?src_:0;
        }
        const char* src_;// 当前读取位置指针
        const char* begin_;// 内存缓冲区的起始位置
        const char* end_;// 内存缓冲区的结束位置
        size_t size_;// 缓冲区总大小
    };
}
#endif