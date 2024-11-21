#ifndef RAPIDJSON_DOCUMENT_H_
#define RAPIDJSON_DOCUMENT_H_

#include "reader.h"
#include "internal/meta.h"
#include "internal/strfunc.h"
#include "memorystream.h"
#include "encodedstream.h"
#include <new>      // placement new
#include <limits>
#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif

#ifndef RAPIDJSON_NOMEMBERITERATORCLASS
#include <iterator> // std::random_access_iterator_tag
#endif

#if RAPIDJSON_USE_MEMBERSMAP
#include <map> // std::multimap
#endif

namespace RAPIDJSON{
    template<typename Encoding, typename Allocator>
    class GenericValue;// 通用版本 JSON值类(这个值不是仅指键值对的值,它也可能是键值对的键)
    template <typename Encoding, typename Allocator, typename StackAllocator>
    class GenericDocument;// 声明一个通用的 JSON 文档类
    // 如果未定义默认分配器,则使用 MemoryPoolAllocator,并以 CrtAllocator 作为其基础分配器
    #ifndef RAPIDJSON_DEFAULT_ALLOCATOR
    #define RAPIDJSON_DEFAULT_ALLOCATOR MemoryPoolAllocator<CrtAllocator>
    #endif
    // 如果未定义默认堆栈分配器,则使用 CrtAllocator
    #ifndef RAPIDJSON_DEFAULT_STACK_ALLOCATOR
    #define RAPIDJSON_DEFAULT_STACK_ALLOCATOR CrtAllocator
    #endif
    // RAPIDJSON::Value 默认分配内存时,允许的对象数量
    #ifndef RAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY
    #define RAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY 16
    #endif
    // RAPIDJSON::Value 默认分配内存时,允许的数组原始数量
    #ifndef RAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY
    #define RAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY 16
    #endif
    /**
     * @brief 生成表示JSON对象的键值对的类
     * 
     * @tparam Encoding 
     * @tparam Allocator 
     */
    template<typename Encoding, typename Allocator>
    class GenericMember{
        public:
            GenericValue<Encoding, Allocator> key;// 键
            GenericValue<Encoding, Allocator> value;// 值
            #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
            // 移动构造函数
            GenericMember(GenericMember&& rhs) RAPIDJSON_NOEXCEPT
                : key(std::move(rhs.key)),
                  value(std::move(rhs.value))
                {}
            // 移动复制操作(对象整体)
            GenericMember& operator=(GenericMember&& rhs) RAPIDJSON_NOEXCEPT{
                return *this = static_cast<GenericMember&>(rhs);
            }
            #endif
            // 移动语义的赋值操作(成员赋值)
            GenericMember& operator=(GenericMember& rhs) RAPIDJSON_NOEXCEPT{
                if(RAPIDJSON_LIKELY(this!=&rhs)){
                    key = rhs.key;
                    value = rhs.value;
                }
                return *this;
            }
            friend inline void swap(GenericMember& a, GenericMember& b) RAPIDJSON_NOEXCEPT{
                a.key.Swap(b.key);
                a.value.Swap(b.value);
            }
        private:
            // 禁止使用拷贝构造函数
            GenericMember(const GenericMember& rhs) = delete;
    };
    #ifndef RAPIDJSON_NOMEMBERITERATORCLASS// 如果未定义该宏,就使用类实现一个成员迭代器,否则会使用指针实现
    /**
     * @brief 定义一个对象成员的迭代器,这个迭代器和C++标准库的迭代器是类似的
     * 
     * @tparam Const 决定是否为常量迭代器 
     * @tparam Encoding 编码方式
     * @tparam Allocator 分配器类型
     */
    template<bool Const, typename Encoding, typename Allocator>
    class GenericMemberIterator{
        friend class GenericValue<Encoding, Allocator>;// 允许GenericValue可以访问GenericMemberIterator的私有成员
        // 声明其它GenericMemberIterator特化版本为友元类
        template<bool, typename, typename> 
        friend class GenericMemberIterator;
        typedef GenericMember<Encoding, Allocator> PlainType;// 不带const限定符的JSON对象成员类型(一个成员->一个键值对)
        typedef typename internal::MaybeAddConst<Const, PlainType>::Type ValueType;// 根据Const决定ValueType是否为const PlainType还是就是PlainType
        public:
            typedef GenericMemberIterator Iterator;// 通用迭代器
            typedef GenericMemberIterator<true, Encoding, Allocator> ConstIterator;// 常量迭代器
            typedef GenericMemberIterator<false, Encoding, Allocator> ConstIterator;// 非常量迭代器
            typedef ValueType   value_type;// 迭代器指向的成员元素类型
            typedef ValueType*  pointer;// 指向成员元素的指针类型
            typedef ValueType&  reference;// 指向成员元素的引用类型
            typedef std::ptrdiff_t difference_type;// 迭代器之间的距离类型(通常为有符号整数)  (如vector中s.begin()和s.begin()+2的距离就是2)
            typedef std::random_access_iterator_tag iterator_category;// 表明是随机访问迭代器
            typedef pointer     Pointer;
            typedef reference   Reference;
            typedef difference_type DifferenceType;
            GenericMemberIterator() : ptr_() {}// 默认构造函数
            GenericMemberIterator(const NonConstIterator& it) : ptr_(it.ptr_) {}// 
            Iterator& operator=(const NonConstIterator& it) {ptr_ = it.ptr_; return *this;}// 重载赋值操作
            // 重载自增和自减运算符
            Iterator& operator++() { ++ptr_; return *this;}
            Iterator& operator--(){--ptr_; return *this;}
            Iterator operator++(int){
                Iterator old(*this);// 用于保存返回上一个迭代器(类似i++,i要保存上一个状态的副本)
                ++ptr_;
                return old;
            }
            Iterator operator--(int){
                Iterator old(*this);// 用于返回后一个迭代器
                --ptr_;
                return old;
            }
            // 重载+、-、+=、-=运算符
            Iterator operator+(DifferenceType n) const {
                return Iterator(pyr_+n);
            }
            Iterator operator-(DifferenceType n) const {
                return Iterator(pyr_-n);
            }
            Iterator& operator+=(DifferenceType n){
                ptr_ += n;
                return *this;
            }
            Iterator& operator-=(DifferenceType n){
                ptr_ -= n;
                return *this;
            }
            // 重载比较操作符
            template <bool Const_> bool operator==(const GenericMemberIterator<Const_, Encoding, Allocator>& that) const { return ptr_ == that.ptr_; }
            template <bool Const_> bool operator!=(const GenericMemberIterator<Const_, Encoding, Allocator>& that) const { return ptr_ != that.ptr_; }
            template <bool Const_> bool operator<=(const GenericMemberIterator<Const_, Encoding, Allocator>& that) const { return ptr_ <= that.ptr_; }
            template <bool Const_> bool operator>=(const GenericMemberIterator<Const_, Encoding, Allocator>& that) const { return ptr_ >= that.ptr_; }
            template <bool Const_> bool operator< (const GenericMemberIterator<Const_, Encoding, Allocator>& that) const { return ptr_ < that.ptr_; }
            template <bool Const_> bool operator> (const GenericMemberIterator<Const_, Encoding, Allocator>& that) const { return ptr_ > that.ptr_; }
            // 解引用操作符
            Reference operator*() const {return *ptr_;}
            Pointer operator->() const {return ptr_;}
            Reference operator[](DifferenceType n) const {return ptr_[n];}
            // 计算两个迭代器之间的距离
            DifferenceType operator-(ConstIterator that) const {return ptr_-that.ptr_;}
        private:
            explicit GenericMemberIterator(Pointer p) : ptr_(p) {}
            Pointer ptr_;// 指向当前迭代器的位置
    };
    #else
        
    #endif



}

#endif