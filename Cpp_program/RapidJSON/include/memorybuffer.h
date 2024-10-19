#ifndef RAPIDJSON_MEMORYBUFFER_H_
#define RAPIDJSON_MEMORYBUFFER_H_

#include "stream.h"
#include "internal/stack.h"

namespace RAPIDJSON{
    /**
     * @brief GenericMemoryBuffer的作用是提供内存中的输出字节流管理,而我们利用栈这个数据结构来管理内存缓冲区  即这个栈stack_实现了GenericMemoryBuffer的内存缓冲区
     * 
     * @tparam Allocator 
     */
    template <typename Allocator = CrtAllocator>
    struct GenericMemoryBuffer{
        typedef char Ch;
        GenericMemoryBuffer(Allocator* allocator=nullptr, size_t capacity=kDefaultCapacity)// nullptr表示默认行为,即Allocator = CrtAllocator
            : stack_(allocator, capacity)// 创建栈管理的内存缓冲区
            {}
        /**
         * @brief 将一个字符c放入内存缓冲区
         * 调用stack_的Push方法在栈上分配一段内存,然后将字符c存储到该内存中
         * 
         * @param c 
         */
        void Put(Ch c) {*stack_.template Push<Ch>() = c;}// template关键字来指明Push()是一个模板成员函数,进而消除歧义
        void Flush() {}
        /**
         * @brief 清空缓冲区数据
         * ShrinkToFit()方法是将缓冲区的容量调整为当前实际数据的大小,节省不必要的内存占用
         */
        void Clear() {stack_.ShrinkToFit();}
        /**
         * @brief 用于在缓冲区中分配count个字节,并返回指向这些字节的指针
         * 
         * @param count 
         * @return Ch* 
         */
        Ch* Push(size_t count) {return stack_.template Push<Ch>(count);}
        /**
         * @brief 从缓冲区中弹出count个字节
         * 
         * @param count 
         */
        void Pop(size_t count) {stack_.template Pop<Ch>(count);}
        /**
         * @brief 返回指向缓冲区底部(起始位置)的指针,返回的是常量指针,即不能通过此指针修改缓冲区内容
         * 
         * @return const Ch* 
         */
        const Ch* GetBuffer() const {
            return stack_.template Bottom<Ch>();
        }
        /**
         * @brief 返回当前缓冲区已使用的字节大小
         * 
         * @return size_t 
         */
        size_t GetSize() const {return stack_.GetSize();}

        static const size_t kDefaultCapacity = 256;
        mutable internal::Stack<Allocator> stack_;// stack_是一个使用Allocator分配器管理的内存栈,用于存储内存缓冲区的数据   stack_就是用来管理内存缓冲区的栈式结构
     };
     typedef GenericMemoryBuffer<> MemoryBuffer;
     /**
      * @brief PutN()是一个为MemoryBuffer提供的特殊优化版本,用于将字符c放入缓冲区中的多个位置,使用memset()函数实现快速填充
      * 
      * @tparam  
      * @param memoryBuffer 
      * @param c 
      * @param n 
      */
     template<>
     inline void PutN(MemoryBuffer& memoryBuffer, char c, size_t n){
        std::memset(memoryBuffer.stack_.Push<char>(n), c, n*sizeof(c));// Push()这个是stack.h对应的函数而不是MemoryBuffer对应的Push
     }
}
#endif