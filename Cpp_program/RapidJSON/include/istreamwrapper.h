#ifndef RAPIDJSON_ISTREAMWRAPPER_H_
#define RAPIDJSON_ISTREAMWRAPPER_H_

#include "stream.h"
#include <iosfwd>// // 引入C++标准库中的输入输出流的前置声明
#include <ios>

namespace RAPIDJSON{
    template <typename StreamType>
    class BasicIStreamWrapper{
        public:
            typedef typename StreamType::char_type Ch;
            /**
             * @brief 根据给定的输入流构建BasicIStreamWrapper对象
             * 
             * @param stream 
             */
            BasicIStreamWrapper(StreamType& stream)
                : stream_(stream),// 绑定传入的流
                  buffer_(peekBuffer_),// 初始化缓冲区为 peekBuffer_
                  bufferSize_(4),// 缓冲区大小为 4
                  bufferLast_(0),// 初始化缓冲区末尾指针
                  current_(buffer_),// 当前读取位置指向缓冲区起始位置
                  readCount_(0),// 读取的字符数初始化为 0
                  count_(0),// 总计读取的字符数初始化为 0
                  eof_(false)
                {
                    Read();// 调用 Read 函数，预读数据到缓冲
                }
            /**
             * @brief 用户提供缓冲区的构造函数
             * 
             * @param stream 
             * @param buffer 
             * @param bufferSize 
             */
            BasicIStreamWrapper(StreamType& stream, char* buffer, size_t bufferSize)
                : stream_(stream),// 绑定传入的流
                  buffer_(buffer),// 初始化缓冲区为buffer
                  bufferSize_(bufferSize),// 缓冲区大小为 4
                  bufferLast_(0),// 初始化缓冲区末尾指针
                  current_(buffer_),// 当前读取位置指向缓冲区起始位置
                  readCount_(0),// 读取的字符数初始化为 0
                  count_(0),// 总计读取的字符数初始化为 0
                  eof_(false)
                {
                    RAPIDJSON_ASSERT(bufferSize >= 4);// 确保缓冲区大小大于等于 4
                    Read();// 调用 Read 函数，预读数据到缓冲
                } 
            /**
             * @brief 查看当前当前缓冲区中读取位置所指向的字符
             * 
             * @return char 
             */
            Ch Peek() const {return *current_;}
            /**
             * @brief 取出当前缓冲区中读取位置的字符,并从下一个字符重新继续读取
             * 
             * @return char 
             */
            Ch Take(){
                Ch c= *current_;
                Read();
                return c;
            }
            /**
             * @brief 返回当前已经从缓冲区中读取走的总字节数
             * 
             * @return size_t 
             */
            size_t Tell() const {
                return count_+static_cast<size_t>(current_-buffer_);// count_表示已经从缓冲区中读取走的字节总数  此时当前缓冲区可能没有被读取完,因此count_还没统计当前缓冲区中被读取的字节数current_-buffer_
            }
            // 以下为未实现的函数，RapidJSON 不需要这些方法
            void Put(Ch) { RAPIDJSON_ASSERT(false); }
            void Flush() { RAPIDJSON_ASSERT(false); } 
            Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
            size_t PutEnd(Ch*) { RAPIDJSON_ASSERT(false); return 0; }
            /**
             * @brief 用于检测  检测是否还剩至少4个字节
             * 
             * @return const char* 
             */
            const Ch* Peek4() const{
                return (current_+4-!eof_ <= bufferLast_)?current_:0;
            }
        private:
            // 禁用拷贝构造函数和拷贝赋值运算符
            BasicIStreamWrapper(const BasicIStreamWrapper&) = delete;
            BasicIStreamWrapper& operator=(const BasicIStreamWrapper&)= delete;
            /**
             * @brief 从输入流中读取数据到缓冲区,并更新相关指针和状态
             * 
             */
            void Read(){
                if(current_ < bufferLast_)// 如果当前指针没有指向缓冲区末尾,则只需将指针前移
                    ++current_;
                else if(!eof_){// 缓冲区被内存读取完了且此时没有读取到流的末尾
                    cout_ += readCount_;
                    readCount_ = bufferSize_;
                    bufferLast_ = buffer_+readCount_-1;
                    current_ = buffer_;// 将当前指针重置为缓冲区的起始位置,以便重新开始读取
                    // 下面的处理类似filereadstream.h中对读取到文件末尾的处理  只是是用流API接口来实现的
                    if(!stream_.read(buffer_, static_cast<std::streamsize>(bufferSize_))){// 从输入流stream_中读取数据到缓冲区buffer_  读取失败(如已到达输入流stream_的末尾或发生读取错误)   这里表示的是到达输入流末尾
                        readCount_ = static_cast<size_t>(stream_.gcount());// 使用 gcount() 方法获取实际读取的字节数
                        *(bufferLast_ = buffer_+readCount_) = '\0';
                        eof_  true;
                    }
                }
            }
            StreamType& stream_;// 输入流
            Ch peekBuffer_[4], *buffer_;// peekBuffer_用于小型缓冲区 buffer_指向用户提供的缓冲区或默认缓冲区
            size_t bufferSize_;// 缓冲区大小
            Ch* bufferLast_;// 指向缓冲区中最后一个字符
            Ch* current_;// 当前读取指针
            size_t readCount_;// 当前缓冲区中已读取的字符数
            size_t count_;// 已经从缓冲区中读取走的字节总数
            bool eof_;// 是否到达流的末尾
    };
    // 定义两种具体的流封装类,一种用于标准输入流,另一种用于宽字符输入流
    typedef BasicIStreamWrapper<std::istream> IStreamWrapper;
    typedef BasicIStreamWrapper<std::wistream> WIStreamWrapper;
}
#endif