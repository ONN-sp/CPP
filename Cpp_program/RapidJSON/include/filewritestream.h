#ifndef RAPIDJSON_FILEWRITESTREAM_H_
#define RAPIDJSON_FILEWRITESTREAM_H_

#include "stream.h"
#include <cstdio>
#include <iostream>

namespace RAPIDJSON{
    class FileWriteStream{
        public:
        /**
         * @brief 初始化FileWriteStream,设置文件指针、缓冲区以及缓冲区大小
         * 
         * @param fp 
         * @param buffer 
         * @param bufferSize 
         */
            FileWriteStream(std::FILE* fp, char* buffer, size_t bufferSize)
                : fp_(fp),
                  buffer_(buffer),
                  bufferEnd_(buffer+bufferSize),
                  current_(buffer_)
                {
                    RAPIDJSON_ASSERT(fp_!=0);// 断言文件指针不为空,确保文件已经打开
                }
            /**
             * @brief 将字符c写入缓冲区,如果缓冲区满了就刷新,然后再将字符c写进缓冲区
             * 
             * @param c 
             */
            void Put(char c){
                if(current_ >= bufferEnd_)// 如果当前指针到达缓冲区的末尾,就刷新缓冲区
                    Flush();
                *current_++ = c;// 将字符写入缓冲区,并更新指针
            }
            /**
             * @brief 写入n个字符(即重复写n次c字符),使用std::memset加快效率
             * 
             * @param c 
             * @param n 
             */
            void PutN(char c, size_t n){
                size_t avail = static_cast<size_t>(bufferEnd_ - current_);// 计算缓冲区中剩余的可用空间
                while(n>avail){// 如果要写入的字符数大于剩余空间,则先填满缓冲区再刷新
                    std::memset(current_, c, avail);// 将缓冲区当前指针位置填充为字符c
                    current_ += avail;// 更新指针
                    Flush();
                    n -= avail;// 减去已写入的字符数
                    avail = static_cast<size_t>(bufferEnd_-current_);// 重新计算剩余空间
                }
                if(n>0){// 如果还有字符需要写入
                    std::memset(current_, c, n);// 填充剩余字符
                    current_ += n;// 更新指针
                }
            }
            /**
             * @brief 刷新缓冲区  将缓冲区中数据->文件中
             * 
             */
            void Flush(){
                if(current_!=buffer_){
                    size_t result = std::fwrite(buffer_, 1, static_cast<size_t>(current_-buffer_), fp_);// 将缓冲区内容写入文件
                    if(result < static_cast<size_t>(current_ - buffer_))// 即没写成功current_-buffer_这么多字节
                        std::cerr << "Error: std::fwrite()!" << std::endl;
                    current_ = buffer_;// 将缓冲区数据写完到文件后,重置当前指针cureent_=缓冲区起始地址
                }
            }
            // 以下未实现的方法,仅为接口需要
            char Peek() const { RAPIDJSON_ASSERT(false); return 0; }  // 不支持Peek
            char Take() { RAPIDJSON_ASSERT(false); return 0; }  // 不支持Take
            size_t Tell() const { RAPIDJSON_ASSERT(false); return 0; }  // 不支持Tell
            char* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }  // 不支持PutBegin
            size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }  // 不支持PutEnd
        private:
            // 禁用拷贝构造函数和拷贝赋值运算符
            FileWriteStream(const FileWriteStream&) = delete;
            FileWriteStream& operator=(const FileWriteStream&)= delete;
            std::FILE* fp_;// 文件指针
            char* buffer_;// 缓冲区起止位置
            char* bufferEnd_;// 缓冲区末尾的位置
            char* current_;// 从缓冲区向文件写数据时,在缓冲区中被写到的位置
    };
}
#endif