#ifndef RAPIJSON_FILEREADSTREAM_H_
#define RAPIJSON_FILEREADSTREAM_H_

#include "stream.h"
#include <cstdio>// 引入 C 标准库中的文件 I/O 相关的头文件 (fread, FILE*)

namespace RAPIDJSON{
    class FileReadStream{
        public:
        /**
         * @brief 初始化FileReadStream对象,设置文件指针、缓冲区以及缓冲区大小
         * 初始化时就会从文件读取一次数据到缓冲区
         * 
         * @param fp 
         * @param buffer 
         * @param bufferSize 
         */
            FileReadStream(std::FILE* fp, char* buffer, size_t bufferSize)
                : fp_(fp),
                buffer_(buffer),
                bufferSize_(bufferSize),
                bufferLast_(0),// 缓冲区末尾为0,表示此时还没有从文件中读取数据到缓冲区
                current_(buffer_),
                readCount_(0),
                count_(0),
                eof_(false)
                {
                    RAPIDJSON_ASSERT(fp_!=0);// 断言文件指针不为空,确保文件已经打开
                    RAPIDJSON_ASSERT(bufferSize >= 4);// 缓冲区大小至少为4字节才行
                    Read();// 初始化时从文件读取一次数据
                }
            /**
             * @brief 查看当前当前缓冲区中读取位置所指向的字符
             * 
             * @return char 
             */
            char Peek() const {return *current_;}
            /**
             * @brief 取出当前缓冲区中读取位置的字符,并从下一个字符重新继续读取
             * 
             * @return char 
             */
            char Take(){
                char c = *current_;
                Read();
                return c;
            }
            /**
             * @brief 返回当前已经从缓冲区中读取走的总字节数
             * 
             * @return size_t 
             */
            size_t Tell() const {
                return count_ + static_cast<size_t>(current_-buffer_);// count_表示已经从缓冲区中读取走的字节总数  此时当前缓冲区可能没有被读取完,因此count_还没统计当前缓冲区中被读取的字节数current_-buffer_
            }
            // 从文件的只读流  没有写操作 所以以下操作不应该被调用
            void Put(char) {RAPIDJSON_ASSERT(false);}
            void Flush() {RAPIDJSON_ASSERT(false);}
            char* PutBegin() {RAPIDJSON_ASSERT(false); return nullptr;}
            size_t PutENd(char*) {RAPIDJSON_ASSERT(false); return 0;}
            const char* Peek4() const {
                return (current_+4-!eof_ <= bufferLast_)? current_:nullptr;
            }
        private:
            /**
             * @brief 从文件中读取数据到缓冲区,并更新相关指针和状态
             * 
             */
            void Read(){
                if(current_ < bufferLast_)// 如果当前指针没有指向缓冲区末尾,则只需将指针前移
                    ++current_;
                else if(!eof_){// 缓冲区被内存读取完了且此时没有读取到文件的末尾
                    count_ += readCount_;// 缓冲区中数据已经被读取完了  因此已经从缓冲区中读取走的字节总数要加上被读取完的缓冲区中的readCount_
                    readCount_ = std::fread(buffer_, 1, bufferSize_, fp_);
                    bufferLast_ = buffer_ + readCount_ -1;
                    current_ = buffer_;
                    if(readCount_ < bufferSize_){// 读完了文件,但是缓冲区还没用完
                        buffer_[readCount_] = '\0';// 添加一个空终止符
                        ++bufferLast_;// 调整末尾指针
                        eof_ = true;// 标记文件结束  
                    }
                }
            }
            std::FILE* fp_;// 文件指针
            char* buffer_;// 用户提供的缓冲区,用于临时存储从文件读取的数据  buffer_表示的是缓冲区的起始位置
            size_t bufferSize_;// 缓冲区大小
            char* bufferLast_;// 缓冲区中最后一个字符的位置
            char* current_;// 当前缓冲区的读取位置的指针
            size_t readCount_;// 本次读取到缓冲区的字符数量
            size_t count_;// 已经从缓冲区中读取走的字节总数
            bool eof_;// 文件是否已经读取完毕(EOF)
    };
}
#endif