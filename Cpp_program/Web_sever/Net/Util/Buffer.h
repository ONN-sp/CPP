#ifndef BUFFER_H
#define BUFFER_H

#include "../../Base/NonCopyAble.h"
#include <vector>
#include <string>

/// Buffer structure: 可以理解为:一块Buffer分为了预留、可读、可写缓冲区
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

namespace tiny_muduo{
    static const int kCheapPrepend = 8;// 预留的前置空间大小
    static const int kInitialSize = 1024;// 缓冲区的初始(容量)大小  初始的时候readIndex=writeIndex
    static const char* kCRLF = "\r\n"; // HTTP协议中的换行符定义
    
    class Buffer : public NonCopyAble{
        public:
            Buffer(int initialSize = kInitialSize);
            ~Buffer() = default;
            int ReadFd(int);// 从文件描述符读取数据到缓冲区  TcpConnection的input buffer就是readFd()读进去的
            char* begin();// 获取缓冲区起始位置的指针
            // 提供常量函数和非常量函数的版本是为了:对于只需要读取数据的函数，可以使用常量函数,从而确保不会意外地修改数据;对于需要修改数据的函数,可以使用非常量函数,以便修改缓冲区的内容
            const char* begin() const;// 获取缓冲区起始位置的指针
            char* beginRead();// 获取当前读索引位置的指针 readIndex_
            const char* beginRead() const;// 获取当前读索引位置的指针 readIndex_
            char* beginWrite();// 获取当前写索引位置的指针
            const char* beginWrite() const;// 获取当前写索引位置的指针
            const char* FindCRLF() const;// 在缓冲区中查找换行符,因为HTTP协议中是
            void Append(const char*);// 追加字符串到缓冲区
            void Append(const char*, int);// 追加指定长度的字符串到缓冲区
            void Append(const std::string&);// 追加std::string格式到缓冲区
            void Retrieve(int);// 从缓冲区中提取指定长度的数据
            void RetrieveUnitilIndex(const char*);// 提取直到指定位置的数据
            void RetrieveAll();// 提取缓冲区所有数据
            int Size(){return buffer_.size();}// 返回此时Buffer的已存储的数据量
            int Capacity(){return buffer_.capacity();}// 返回当前缓冲区的总存储容量,即缓冲区可以容纳的最大字节数  没有超过这个容量,就不会重新分配内存
            std::string RetrieveAsString(int);// 从缓冲区中提取指定长度的数据,并返回一个std::string
            std::string RetrieveAllAsString();// 从缓冲区中提取所有的数据,并返回一个std::string
            const char* Peek() const;// 获取当前读位置的指针  readIndex_
            char* Peek();// 获取当前读位置的指针  
            int readablebytes() const;// 获取可读字节数 readable
            int writeablebytes() const;// 获取可写字节数 writable
            int prependablebytes() const;// 获取可预留字节数  prependable
            bool MakeSureEnoughStorage(int);// 确保缓冲区有足够的存储空间
        private:
            std::vector<char> buffer_;// 底层的字节缓冲区
            int readIndex_;// 当前的读索引(这是下标,不是指针)
            int writeIndex_;// 当前的写索引(这是下标,不是指针)
    };
}

#endif
