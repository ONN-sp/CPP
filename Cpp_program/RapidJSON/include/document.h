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
    class GenericValue;// 通用版本JSON值类(这个值不是仅指键值对的值,它也可能是键值对的键)
    template <typename Encoding, typename Allocator, typename StackAllocator>
    class GenericDocument;// 声明一个表示JSON文档的通用的JSON文档类
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
     * @brief 表示一个JSON对象的键值对的类
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
     * @brief 表示一个对象成员的迭代器,这个迭代器和C++标准库的迭代器是类似的
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
    template<bool Const, typename Encoding, typename Allocator>
    class GenericMemberIterator;
    /**
     * @brief 非常量模板的特化
     * 直接使用裸指针GenericMember<Encoding, Allocator>*作为底层实现,而不是用迭代器
     * @tparam Encoding 
     * @tparam Allocator 
     */
    template<typename Encoding, typename Allocator>
    class GenericMemberIterator<false, Encoding, Allocator>{
        public:
            typedef GenericMember<Encoding, Allocator>* Iterator;
            template<typename Encoding, typename Allocator>
            class GenericMemberIterator<true, Encoding, Allocator>{
                public:
                    typedef const GenericMember<Encoding, Allocator>* Iterator;
            };
    };
    #endif
    /**
     * @brief 用于封装对字符串的引用,允许高效、安全地操作字符串,避免了内存管理的额外开销
     * 
     * @tparam CharType 
     */
    template<typename CharType>
    struct GenericStringRef{
        typedef CharType Ch;
        template<SizeType N>
        // 利用固定长度的字符数组封装字符串的引用
        GenericStringRef(const CharType(&str)[N]) RAPIDJSON_NOEXCEPT
            : s(str),
              length(N-1)
            {}
        // 利用字符串指针封装字符串的引用
        explicit GenericStringRef(const CharType* str)
            : s(str),
              length(NotNullStrLen(str))
            {}
        // 利用字符指针和长度len封装字符串的引用
        GenericStringRef(const CharType* str, SizeType len)
            : s(RAPIDJSON_LIKELY(str) ? str:emptyString),
              length(len)
            {RAPIDJSON_ASSERT(str!=0 || len==0u);}
        // 拷贝构造函数
        GenericStringRef(const GenericStringRef& rhs)
            : s(rhs.s),
              length(rhs.length)
            {}
        // 将GenericStringRef隐式转换为普通字符指针
        operator const Ch* () const {return s;}
        const Ch* const s;
        const SizeType length;
        private:
            // 计算非空字符串str的长度
            SizeType NotNullStrLen(const CharType* str){
                RAPIDJSON_ASSERT(str!=0);
                return internal::StrLen(str);
            }
            static const Ch emptyString[];// 定义一个静态常量空字符串emptyString,用于在输入字符串为空时提供默认值
            template<SizeType N>
            GenericStringRef(CharType (&str)[N]) = delete;
            GenericStringRef& operator=(const GenericStringRef& rhs) = delete;
    };
    /**
     * @brief 提供一个简单的函数用于将普通字符指针包装为GenericStringRef对象
     * 
     * @tparam CharType 
     * @param str 
     * @return GenericStringRef<CharType> 
     */
    template<typename CharType>
    inline GenericStringRef<CharType> StringRef(const CharType* str){
        return GenericStringRef<CharType>(str);
    }
    template<typename CharType>
    inline GenericStringRef<CharType> StringRef(const CharType* str, size_t length){
        return GenericStringRef<CharType>(str, SizeType(length));
    }
    #if RAPIDJSON_HAS_STDSTRING
    template<typename CharType>
    inline GenericStringRef<CharType> StringRef(const std::basic_string<CharType>7 str){
        return GenericStringRef<CharType>(str.data(), SizeType(str.size()));
    }
    #endif
    namespace internal{
        /**
         * @brief 定义一个继承于FalseType的通用模板,表示T不是GenericValue类型
         * 
         * @tparam T 
         * @tparam Encoding 
         * @tparam Allocator 
         */
        template<typename T, typename Encoding=void, typename Allocator=void>
        struct IsGenericValueImpl : FalseType {};
        /**
         * @brief 用于检测T是否是GenericValue的派生类型或T是否就是GenericValue类型
         * 若判断成功则返回TrueType
         * 
         * @tparam T 
         */
        template<typename T> struct IsGenericValueImpl<T, typename Void<typename T::EncodingType>::Type, typename Void<typename T::AllocatorType>::Type>
            : IsBaseOf<GenericValue<typename T::EncodingType, typename T::AllocatorType>, T>::Type {};
        template<typename T> struct IsGenericValue : IsGenericValueImpl<T>::Type {};// 简化版本
    }
    namespace internal{
        /**
         * @brief 通用类型
         * 
         * @tparam ValueType 
         * @tparam T 
         */
        template<typename ValueType, typename T>
        struct TypeHelper{};
        /**
         * @brief 为不同类型(布尔、整数、浮点、字符串等)提供统一的接口,用于检查类型(Is())、获取值(Get())、设置值(Set())
         * 特化版本:bool类型
         * @tparam ValueType 
         */
        template<typename ValueType>
        struct TypeHelper<ValueType, bool>{
            static bool Is(const ValueType& v) {return v.IsBool();}// 检测v是否是布尔类型
            static bool Get(const ValueType& v) {return v.GetBool();}// 从v中获取布尔值
            static ValueType& Set(ValueType& v, bool data) {return v.SetBool(data);}// 设置布尔值data到v中
            static ValueType& Set(ValueType& v, bool data, typename ValueType::AllocatorType&) {return v.SetBool(data);}// 重载方法,允许传入分配器,即使布尔类型不需要分配内存,重载是为了统一接口
        };
        /**
         * @brief 特化版本:整数int类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType> 
        struct TypeHelper<ValueType, int> {
            static bool Is(const ValueType& v) { return v.IsInt(); }
            static int Get(const ValueType& v) { return v.GetInt(); }
            static ValueType& Set(ValueType& v, int data) { return v.SetInt(data); }
            static ValueType& Set(ValueType& v, int data, typename ValueType::AllocatorType&) { return v.SetInt(data); }
        };
        /**
         * @brief 特化版本:无符号unsigned类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType> 
        struct TypeHelper<ValueType, unsigned> {
            static bool Is(const ValueType& v) { return v.IsUint(); }
            static unsigned Get(const ValueType& v) { return v.GetUint(); }
            static ValueType& Set(ValueType& v, unsigned data) { return v.SetUint(data); }
            static ValueType& Set(ValueType& v, unsigned data, typename ValueType::AllocatorType&) { return v.SetUint(data); }
        };
        /**
         * @brief 特化版本:64位整数int64_t类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType> 
        struct TypeHelper<ValueType, int64_t> {
            static bool Is(const ValueType& v) { return v.IsInt64(); }
            static int64_t Get(const ValueType& v) { return v.GetInt64(); }
            static ValueType& Set(ValueType& v, int64_t data) { return v.SetInt64(data); }
            static ValueType& Set(ValueType& v, int64_t data, typename ValueType::AllocatorType&) { return v.SetInt64(data); }
        };
        /**
         * @brief 特化版本:64位无符号整数uint64_t类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType> 
        struct TypeHelper<ValueType, uint64_t> {
            static bool Is(const ValueType& v) { return v.IsUint64(); }
            static uint64_t Get(const ValueType& v) { return v.GetUint64(); }
            static ValueType& Set(ValueType& v, uint64_t data) { return v.SetUint64(data); }
            static ValueType& Set(ValueType& v, uint64_t data, typename ValueType::AllocatorType&) { return v.SetUint64(data); }
        };
        /**
         * @brief 特化版本:双精度浮点数double类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType> 
        struct TypeHelper<ValueType, double> {
            static bool Is(const ValueType& v) { return v.IsDouble(); }
            static double Get(const ValueType& v) { return v.GetDouble(); }
            static ValueType& Set(ValueType& v, double data) { return v.SetDouble(data); }
            static ValueType& Set(ValueType& v, double data, typename ValueType::AllocatorType&) { return v.SetDouble(data); }
        };
        /**
         * @brief 特化版本:单精度浮点数float类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType> 
        struct TypeHelper<ValueType, float> {
            static bool Is(const ValueType& v) { return v.IsFloat(); }
            static float Get(const ValueType& v) { return v.GetFloat(); }
            static ValueType& Set(ValueType& v, float data) { return v.SetFloat(data); }
            static ValueType& Set(ValueType& v, float data, typename ValueType::AllocatorType&) { return v.SetFloat(data); }
        };
        /**
         * @brief 特化版本:字符串Ch*类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType> 
        struct TypeHelper<ValueType, const typename ValueType::Ch*> {
            typedef const typename ValueType::Ch* StringType;
            static bool Is(const ValueType& v) { return v.IsString(); }
            static StringType Get(const ValueType& v) { return v.GetString(); }
            static ValueType& Set(ValueType& v, const StringType data) { return v.SetString(typename ValueType::StringRefType(data)); }
            static ValueType& Set(ValueType& v, const StringType data, typename ValueType::AllocatorType& a) { return v.SetString(data, a); }
        };
        #if RAPIDJSON_HAS_STDSTRING
        template<typename ValueType>
        struct TypeHelper<ValueType, std::basic_string<typename ValueType::Ch>>{
            typedef std::basic_string<typename ValueType::Ch> StringType;
            static bool Is(const ValueType& v) {return v.IsString();}
            static StringType Get(const ValueType& v) {return StringType(v.GetString(), v.GetStringLength());}// 使用std::basic_string构造函数,传入指针和长度,安全地构建一个 std::string或std::wstring
            static ValueType& Set(ValueType& v, const StringType& data, typename ValueType::AllocatorType& a) { return v.SetString(data, a); }
        };
        #endif
        /**
         * @brief 特化版本:数组Array类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::Array>{
            typedef typename ValueType::Array ArrayType;
            static bool Is(const ValueType& v) {return v.IsArray();}
            static ArrayType Get(ValueType& v) {return v.GetArray();}
            static ValueType& Set(ValueType& v, ArrayType data) {return v=data;}
            static ValueType& Set(ValueType& v, ArrayType data, typename ValueType::AllocatorType&) {return v=data;}
        };
        /**
         * @brief 特化版本:不可变(常量)数组ConstArray类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::ConstArray>{
            typedef typename ValueType::ConstArray ArrayType;
            static bool Is(const ValueType& v) {return v.IsArray();}
            static ArrayType Get(const ValueType& v) {return v.GetArray();}
        };
        /**
         * @brief 特化版本:对象Object类型(Object表示JSON的对象)
         * 
         * @tparam ValueType 
         */
        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::Object>{
            typedef typename ValueType::Object ObjectType;
            static bool Is(const ValueType& v) {return v.IsObject();}
            static ObjectType Get(ValueType& v) {return v.GetObject();}
            static ValueType& Set(ValueType& v, ObjectType data) {return v=data;}
            static ValueType& Set(ValueType& v, ObjectType data, typename ValueType::AllocatorType&) {return v=data;}
        };
        /**
         * @brief 特化版本:不可变(常量)对象ConstObject类型
         * 
         * @tparam ValueType 
         */
        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::ConstObject>{
            typedef typename ValueType::ConstObject ObjectType;
            static bool Is(const ValueType& v) {return v.IsObject();}
            static ObjectType Get(const ValueType& v) {return v.GetObject();}
        };
    }
    template<bool, typename> class GenericArray;// 生成JSON的数组
    template<bool, typename> class GenericObject;// 生成JSON的对象
    /**
     * @brief 表示一个JSON值,可以是任意JSON数据类型(这里不是指的对象中的键值对中的值)
     * 
     * @tparam Encoding 
     * @tparam Allocator 
     */
    template<typename Encoding, typename Allocator=RAPIDJSON_DEFAULT_ALLOCATOR>
    class GenericValue{
        public:
            // 增强代码的可维护性,定义类型的别名
            typedef GenericMember<Encoding, Allocator> Member;// 表示JSON对象中的键值对(key-value),由GenericMember类实现
            typedef Encoding EncodingType;
            typedef Allocator AllocatorType;
            typedef typename Encoding::Ch Ch;
            typedef GenericStringRef<Ch> StringRefType;// 让StringRefType表示一个常量字符串的引用
            typedef typename GenericMemberIterator<false, Encoding, Allocator>::Iterator MemberIterator;// 表示用于迭代JSON对象成员的可变迭代器
            typedef typename GenericMemberIterator<true, Encoding, Allocator>::Iterator ConstMemberIterator;// 表示常量迭代器
            typedef GenericValue* ValueIterator;
            typedef const GenericValue* ConstValueIterator;
            typedef GenericValue<Encoding, Allocator> ValueType;
            typedef GenericArray<false, ValueType> Array;
            typedef GenericArray<true, ValueType> ConstArray;
            typedef GenericObject<false, ValueType> Object;
            typedef GenericObject<true, ValueType> ConstObject;
            // data_是内部存储数据的成员,初始标志设置为null类型
            GenericValue() RAPIDJSON_NOEXCEPT : data_() {data_.f.flags = kNullFlag;}
            #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
                GenericValue(GenericValue&& rhs) RAPIDJSON_NOEXCEPT : data_(rhs.data_){
                    rhs.data_.f.flags = kNullFlag;
                }
            #endif
            private:
                GenericValue(const GenericValue& rhs) = delete;// 禁止拷贝构造
            #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
                // 允许通过移动操作从一个GenericDocument对象中创建一个新的GenericValue对象
                template<typename StackAllocator>
                GenericValue(GenericDocument<Encoding, Allocator, StackAllocator>&& rhs);
                template<typename StackAllocator>
                GenericValue& operator=(GenericDocument<Encoding, Allocator, StackAllocator>&& rhs);
            #endif
    };
    


}

#endif