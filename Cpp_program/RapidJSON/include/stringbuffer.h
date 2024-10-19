#ifndef RAPIDJSON_STRINGBUFFER_H_
#define RAPIDJSON_STRINGBUFFER_H_

#include "stream.h"
#include "internal/stack.h"

#if RAPIDJSON_HAS_CXX11_RVALUE_REFS
#include <utility>// std::move
#endif

namespace RAPIDJSON{
    template <typename Encoding, typename Allocator = CrtAllocator>
    class GenericStringBuffer{
        public:
            typedef typename Encoding::Ch Ch;// 如:UTF8::Ch  typename用于消除依赖类型中的歧义,即指明Ch是依赖于模板参数Encoding的类型,而不是变量或静态成员
            /**
             * @brief GenericStringBuffer的作用是提供内存中的输出字节流管理,而我们利用栈这个数据结构来管理字符串缓冲区  即这个栈stack_实现了GenericStringBuffer的内存缓冲区
             * 
             * @param allocator 
             * @param capacity 
             */
            GenericStringBuffer(Allocator* allocator=nullptr, size_t capacity=kDefaultCapacity)
                : stack_(allocator, capacity)
            {}
            #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
            GenericStringBuffer(GenericStringBuffer&& rhs)// 移动构造,即会调用构造函数
                : stack_(std::move(rhs.stack_))
            {}
            GenericStringBuffer& operator=(GenericStringBuffer&& rhs){// 移动赋值,即会使用=
                if(&rhs!=this)
                    stack_ = std::move(rhs.stack_);
                return *this;
            }
            #endif
            /**
             * @brief 将一个字符放入由stack_管理的字符串缓冲区
             * 
             * @param c 
             */
            void Put(Ch c) {*stack_.template Push<Ch>() = c;}// stack_.template:调用stack_的模板成员函数Push()
            /**
             * @brief 不检查空间是否足够  直接将字符写入缓冲区(性能高但不安全)
             * 
             * @param c 
             */
            void PutUnsafe(Ch c) {*stack_.template PushUnsafe<Ch>() = c;}
            /**
             * @brief 不涉及缓冲区->本地  的操作
             * 
             */
            void Flush() {}
            /**
             * @brief 清空缓冲区数据
             * 
             */
            void Clear() {stack_.Clear();}// stack_.Clear()不是模板成员函数,而是普通成员函数  所以没用template
            /**
             * @brief 调整缓冲区大小  使其刚好容纳现有数据(优化内存占用)
             * 
             */
            void ShrinkToFit(){
                *stack_.template Push<Ch>() = '\0';
                stack_.ShrinkToFit();
                stack_.template Pop<Ch>(1);
            }
            /**
             * @brief 预留一定的内存大小
             * 
             * @param count 
             */
            void Reserve(size_t count) {stack_.template Reserve<Ch>(count);}
            /**
             * @brief 向缓冲区中插入count个字符
             * 
             * @param count 
             * @return Ch* 
             */
            Ch* Push(size_T count) {return stack_.template Push<Ch>(count);}
            /**
             * @brief 不检查空间是否足够的前提下向缓冲区插入count个字符
             * 
             * @param count 
             * @return Ch* 
             */
            Ch* PushUnsafe(size_t count) {return stack_.template PushUnsafe<Ch>(count);}
            /**
             * @brief 从缓冲区中弹出count个字符
             * 
             * @param count 
             */
            void Pop(size_t count) {stack_.template Pop<Ch>(count);}
            /**
             * @brief 获取缓冲区中的字符串指针
             * 返回以'\0'结尾的字符串
             * 
             * @return const Ch* 
             */
            const Ch* GetString() const {
                *stack_.template Push<Ch>() = '\0';// Push()返回栈顶指针
                stack_.template Pop<Ch>(1);// 仅移动栈顶指针
                return stack_.template Bottom<Ch>();// 返回栈底,即缓冲区头部指针
            }
            /**
             * @brief 获取字符串的大小(以字节为单位)
             * 
             * @return size_t 
             */
            size_t GetSize() const {return stack_.GetSize();}
            /**
             * @brief 获取字符串的长度(以字符数为单位)
             * 
             * @return size_t 
             */
            size_t GetLength() const {return stack_.GetSize()/sizeof(Ch);}
            static const size_t kDefaultCapacity = 256;
            mutable internal::Stack<Allocator> stack_;
        private:
            // 禁用拷贝构造函数和拷贝赋值运算符
            GenericStringBuffer(const GenericStringBuffer&) = delete;
            GenericStringBuffer& operator=(const GenericStringBuffer&)= delete;
    };
    typedef GenericStringBuffer<UTF8<>> StringBuffer;// 默认使用UTF8编码格式
    /**
     * @brief 预留一定数量的内存空间,避免后续写入时频繁扩展缓冲区
     * 虽然GenericStringBuffer::Reserve()可以实现相同功能,但是PutReserve()提供了更高层次的抽象,用户可以直接调用PutReserve()
     * @tparam Encoding 
     * @tparam Allocator 
     * @param stream 
     * @param count 
     */
    template<typename Encoding, typename Allocator>
    inline void PutReserve(GenericStringBuffer<Encoding, Allocator>& stream, size_t count){
        stream.Reserve(count);
    }
    /**
     * @brief 对GenericStringBuffer::PutUnsafe()的更高层次API接口的封装
     * 
     * @tparam Encoding 
     * @tparam Allocator 
     * @param stream 
     * @param c 
     */
    template<typename Encoding, typename Allocator>
    inline void PutUnsafe(GenericStringBuffer<Encoding, Allocator>& stream, typename Encoding::Ch c){
        stream.PutUnsafe(c);
    }
    /**
     * @brief 仅针对UTF8编码的特殊化实现,用std::memset()来优化多个重复字符的插入
     * 
     * @tparam  
     * @param stream 
     * @param c 
     * @param n 
     */
    template<>
    inline void PutN(GenericStringBuffer<UTF8<>>& stream, char c, size_t n){
        std::memset(stream.stack_.Push<char>(n), c, n*sizeof(c));
    }
}
#endif