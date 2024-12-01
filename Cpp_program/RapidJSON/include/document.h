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
            typedef GenericMemberIterator<false, Encoding, Allocator> NonConstIterator;// 非常量迭代器
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
     * @brief 提供一个简单的函数用于将普通字符指针包装为GenericStringRef对象,其实就是GenericStringRef类的上层封装
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
    inline GenericStringRef<CharType> StringRef(const std::basic_string<CharType>& str){
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
            public:
            /**
             * @brief 
             * 
             * @param type 表示的是需要构造的JSON值的类型
             */
                explicit GenericValue(Type type) RAPIDJSON_NOEXCEPT : data_() {
                    // 定义一个与JSON类型对应的标志位数组,即第一个表示的是null类型,所以defaultFlags[0]=kNullFlag
                    static const uint16_t defaultFlags[] = {
                        kNullFlag, kFalseFlag, kTrueFlag, kObjectFlag, kArrayFlag, kShortStringFlag,
                        kNumberAnyFlag
                    };
                    RAPIDJSON_NOEXCEPT_ASSERT(type >= kNullType && type <= kNumberType);
                    data_.f.flags = defaultFlags[type];// 设置标志位
                    if(type==kStringType)
                        data_.ss.SetLength(0);// 如果type是字符串类型,则使用ShortString存储空字符串
                }
                /**
                 * @brief 拷贝构造函数,用于从GenericValue的另一个实例构造
                 * 
                 * @tparam SourceAllocator 
                 * @param rhs 
                 * @param allocator 
                 * @param copyConstStrings 决定是否复制常量字符串
                 */
                template<typename SourceAllocator>
                GenericValue(const GenericValue<Encoding, SourceAllocator>& rhs, Allocator& allocator, bool copyConstStrings=false){
                    switch(rhs.GetType()){
                        case kObjectType:// 如果rhs的类型是对象,则调用DoCopyMembers函数复制成员
                            DoCopyMembers(rhs, allocator, copyConstStrings);
                            break;
                        case kArrayType:{// 如果rhs的类型是数组,则分配内存并依次复制数组中的每个元素
                            SizeType count = rhs.data_.a.size;
                            GenericValue* le = reinterpret_cast<GenericValue*>(allocator.Malloc(count*sizeof(GenericValue)));// 分配rhs表示的数组一样大小的内存
                            const GenericValue<Encoding, SourceAllocator>* re = rhs.GetElementsPointer();// 获取rhs中的数组头指针,便于后续复制
                            for(SizeType i=0;i<count;i++)
                                new (&le[i]) GenericValue(re[i], allocator, copyConstStrings);// 在新分配的内存中依次构造数组的每个元素
                            data_.f.flags = kArrayFlag;// 设置数组标志
                            data_.a.size = data_.a.capacity = count;// 设置数组的大小和容量
                            SetElementsPointer(le);// 设置新数组的指针
                        }
                        break;
                    case kStringType:// 字符串类型
                        if(rhs.data_.f.flags == kConstStringFlag&& !copyConstStrings){// 非常量字符串
                            data_.f.flags = rhs.data_.f.flags;
                            data_ = *reinterpret_cast<const Data*>(&rhs.data_);
                        }
                        else// 常量字符串
                            SetStringRaw(StringRef(rhs.GetString(), rhs.GetStringLength()), allocator);
                        break;
                    default:
                        data_.f.flags = rhs.data_.f.flags;
                        data_ = *reinterpret_cast<const Data*>(&rhs.data_);
                        break;
                    }
                }
                // 布尔类型的JSON值构建
                #ifndef RAPIDJSON_DOXYGEN_RUNNING
                    template<typename T>// 确保T必须是bool类型
                    explicit GenericValue(T b, RAPIDJSON_ENABLEIF((internal::IsSame<bool, T>>))) RAPIDJSON_NOEXCEPT
                #else
                    explicit GenericValue(bool b) RAPIDJSON_NOEXCEPT
                #endif
                        : data_(){
                            RAPIDJSON_STATIC_ASSERT((internal::IsSame<bool, T>::Value));
                            data_.f.flags = b ? kTrueType :: kFalseType;
                        }
                // int类型的JSON值对象构造
                explicit GenericValue(int i) RAPIDJSON_NOEXCEPT : data_() {
                    data_.n.i64 = i;
                    data_.f.flags = (i>0) ? (kNumberIntFlag | kUintFlag | kUint64Flag) : kNumberIntFlag;// 如果i是非负数则可以标记为既是整数也是无符号整数,且可以用Uint64_tr处理;否则仅标记为kNumberIntFlag
                }
                // uint无符号类型的JSON值对象构造
                explicit GenericValue(unsigned u) RAPIDJSON_NOEXCEPT : data_() {
                    data_.n.u64 = u;
                    data.f.flags = (u&0x80000000) ? kNumberUintFlag : (kNumberUintFlag|kIntFlag|kInt64Flag);// (u&0x80000000)取的是u的最高位,若最高位为1那么只设置kNumberUintFlag标志(因为此时的u就超过了int可以表示的正数范围,即此时u不能用int表示了);若最高位为0,则此时u可以视为int来处理(因为int包括了此时u的范围),那么此时可以被视为无符号整数标志、带符号整数标志或int标志
                }
                // int64_t类型的JSON值对象构造
                explicit GenericValue(int64_t i64) RAPIDJSON_NOEXCEPT data_(){
                    data_.n.i64 = i64;
                    data_.f.flags = kNumberInt64Flag;// 设置64位整数标记
                    if(i64>=0){// 非负的64位整数,此时int64可以视为uint64来处理
                        data.f.flags |= kNumberUint64Flag;// 设置无符号uint64_t标记
                        if(!(static_cast<uint64_t>(i64)&RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0x00000000)))// 查看i64的高32位是否为0,若为0则i64在uint32范围内
                            data.f.flags |= kUintFlag;// 设置为uint无符号
                        if(!(static_cast<uint64_t>(i64)&RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0x80000000)))// 查看i64的高32位和低32位,高32位=0,低32位的第一位=0,此时在int32有符号范围内
                            data_.f.flags |= kIntFlag;// 设置为int有符号
                    }
                    else if(i64 >= static_cast<int64_t>(RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0x80000000)))// i64<0且在int32有符号范围内
                        data_.f.flags |= kIntFlag;// 设置为有符号
                }
                // uint64_t类型的JSON值对象构造
                explicit GenericValue(uint64_t u64) RAPIDJSON_NOEXCEPT : data_(){
                    data.n.u64 = u64;
                    data_.f.flags = kNumberUint64Flag;// 设置为64位无符号整数标记
                    if(!(u64&RAPIDJSON_UINT64_C2(0x80000000, 0x00000000)))// u64高位为0,此时的uint64可以视为int64来处理
                        data_.f.flags |= kInt64Flag;
                    if(!(u64&RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0x00000000)))// u64低32位为0,设置为无符号uint32标记
                        data_.f.flags |= kUintFlag;
                    if(!(u64&RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0x80000000)))// u64低32位为1,设置为有符号int32标记
                        data_.f.flags |= kIntFlag;
                }
                // double类型的JSON值对象构造
                explicit GenericValue(double d) RAPIDJSON_NOEXCEPT : data_() {
                    data_.n.d = d;
                    data_.f.flags = kNumberDoubleFlag;
                }
                // float类型的JSON值对象构造
                explicit GenericValue(float f) RAPIDJSON_NOEXCEPT : data_() {
                    data_.n.d = static_cast<double>(f>;
                    data_.f.flags = kNumberDoubleFlag;
                }
                // 常量字符串const Ch*的JSON值对象构造
                GenericValue(const Ch* s, SizeType length) RAPIDJSON_NOEXCEPT : data_(){
                    SetStringRaw(StringRef(s, length));// 设置标记的操作隐藏在SetStringRaw()中
                }
                // 常量字符串对象StringRefType的JSON值对象构造
                explicit GenericValue(StringRefType s) RAPIDJSON_NOEXCEPT : data_() {
                    SetStringRaw(s);
                }
                // 带分配器的常量字符串const Ch*的JSON值对象构造
                GenericValue(const Ch* s, SizeType length, Allocator& allocator) : data_() {
                    SetStringRaw(StringRef(s, length), allocator);
                }
                // 带分配器的常量字符串const Ch*的JSON值对象构造(无指定长度)
                GenericValue(const Ch* s, Allocator& allocator) : data_() {
                    SetStringRaw(StringRef(s), allocator);
                }
                // 将C++标准库的std::basic_string<Ch>的字符串JSON值对象构造
                #if RAPIDJSON_HAS_STDSTRING
                    GenericValue(const std::basic_string<Ch>& s, Allocator& allocator) : data_() {
                        SetStringRaw(StringRef(s), allocator);
                    }
                #endif
                // 将一个Array类型的对象转换位GenericValue对象.GenericValue可以直接表示一个JSON数组,所以这里是将用Array表示的JSON数组用更通用的GenericValue来表示
                GenericValue(Array a) RAPIDJSON_NOEXCEPT : data_(a.value_.data_){// Array.value_是一个GenericValue对象,因此它有data_成员
                    a.value_.data_ = Data();// 清空Array对象的内部数据
                    a.value_.data_.f.flags = kArrayFlag;
                }
                // 将一个Object类型的对象转换位GenericValue对象
                GenericValue(Object o) RAPIDJSON_NOEXCEPT : data_(o.value_.data_)P{
                    o.value_.data_ = Data();// 清空Object对象的内部数据
                    o.value_.data_.f.flags = kObjectFlag;
                }
                ~GenericValue(){
                    if(Allocator::kNeedFree || (RAPIDJSON_USE_MEMBERSMAP+0 &&
                                                internal::IsRefCounted<Allocator>::Value)){// 后半部分表示是否使用计数机制来管理内存
                        switch(data_.f.flags){
                            case kArrayFlag:
                                {
                                    GenericValue* e = GetElementsPointer();
                                    for(GenericValue* v=e; v!=e+data_.a.size;++v)
                                        v->~GenericValue();// 一个数组元素一个数组元素的析构
                                    if(Allocator::kNeedFree)
                                        Allocator::Free(e);
                                }
                                break;
                            case kObjectFlag:
                                DoFreeMembers();// 负责释放对象中的成员(键值对)
                                break;
                            case kCopyStringFlag:
                                if(Allocator::kNeedFree)
                                    Allocator::Free(const_cast<Ch*>(GetStringPointer()));// 释放data_中的s.str
                                break;
                            default:
                                break;
                        }
                    }
                }
                // 重载赋值操作符  拷贝赋值
                GenericValue& operator=(GenericValue& rhs) RAPIDJSON_NOEXCEPT{
                    if(RAPIDJSON_LIKELY(this!=&rhs)){
                        GenericValue temp;
                        temp.RawAssign(rhs);// 将rhs的赋给对象temp
                        this->~GenericValue();// 在给当前对象this赋值之前先释放它的资源
                        RawAssign(temp);// 将临时对象temp赋给当前对象this
                    }
                    return *this;
                }
                // C++11的右值引用赋值操作符
                #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
                    GenericValue& operator=(GenericValue&& rhs) RAPIDJSON_NOEXCEPT{
                        return *this = rhs.Move();// 调用rhs.Move()将右值rhs转换为左值,然后使用前面重载的拷贝赋值操作符将其赋给当前对象this
                    }
                #endif
                // 重定义一个字符串引用StringRefType直接赋值给GenericValue对象的赋值操作,即 GenericValue<=StringRefType
                GenericValue& operator=(StringRefType str) RAPIDJSON_NOEXCEPT{
                    GenericValue s(str);
                    return *this = s;
                }
                template<typename T>
                // 禁用指针类型的赋值,只有非指针类型的T才能直接被赋值到GenericValue对象
                RAPIDJSON_DISABLEIF_RETURN((internal::IsPointer<T>), (GenericValue&))
                operator=(T value){
                    GenericValue v(value);
                    return *this = v;
                }
                // 深拷贝(为新对象分配新的内存)赋值,用于从rhs拷贝内容到当前对象
                template<typename SourceAllocator>
                GenericValue& CopyFrom(const GenericValue<Encoding, SourceAllocator>& rhs, Allocator& allocator, bool copyConstStrings=false){
                    RAPIDJSON_ASSERT(static_cast<void*>(this) != static_cast<void const *>(&rhs));
                    this->~GenericValue();// 拷贝前先释放当前对象的资源.这样做是为了确保在拷贝数据之前,当前对象不再持有任何资源,防止资源泄漏
                    new (this) GenericValue(rhs, allocator, copyConstStrings);// 使用placement new在当前对象内存中重新构造一个GenericValue对象
                    return *this;
                }
                // 交换赋值 other对象与当前对象交换
                GenericValue& Swap(GenericValue& other) RAPIDJSON_NOEXCEPT{
                    GenericValue temp;
                    temp.RawAssign(*this);
                    RawAssign(other);
                    other.RawAssign(temp);
                    return *this;
                }
                friend inline void swap(GenericValue& a, GenericValue& b) RAPIDJSON_NOEXCEPT {a.Swap(b);}
                // 返回当前对象的引用
                GenericValue& Move() RAPIDJSON_NOEXCEPT {return *this;}
                /**
                 * @brief 重载与另一个GenericValue对象相等判断==
                 * 
                 * @tparam SourceAllocator 
                 * @param rhs 
                 * @return true 
                 * @return false 
                 */
                template<typename SourceAllocator>
                bool operator==(const GenericValue<Encoding, SourceAllocator>& rhs) const {
                    typedef GenericValue<Encoding, SourceAllocator> RhsType;
                    if(GetType() != rhs.GetType())// 1. 类型相同
                        return false;
                    switch(GetType()){// 2. 通过不同类型进一步判断大小、值等是否相同(不同类型比较的东西不同)
                        case kObjectType:
                            if(data_.o.size != rhs.data_.o.size)
                                return false;
                            for(ConstMemberIterator lhsMemberItr = MemberBegin(); lhsMemberItr!=MemberEnd();++lhsMemberItr){
                                typename RhsType::ConstMemberIterator rhsMemberItr = rhs.FindMember(lhsMemberItr->name);// 对于对象成员来说,找匹配的name,找不到就会返回到MemberEnd()
                                if(rhsMemberItr==rhs.MemberEnd() || (!(lhsMemberItr->value==rhsMemberItr->value)))// 判断key和value是否都能匹配
                                    return false;
                            }
                            return true;
                        case kArrayType:
                            if(data_.a.size != rhs.data_.a.size)
                                return false;
                            for(SizeType i=0;i<data_.a.size;++i)
                                if(!((*this)[i]==rhs[i]))
                                    return false;
                            return true;
                        case kStringType:
                            return StringEqual(rhs);
                        case kNumberType:
                            if(IsDouble() || rhs.IsDouble()){
                                double a= GetDouble();
                                double b = rhs.GetDouble();
                                return a>=b &&a<=b;// a==b才返回true
                            }
                            else    
                                return data_.n.u64 == rhs.data_.n.u64;
                        default:
                            return true;
                    }
                }
                // 当前GenericValue对象与Ch字符串直接判断是否相等的重载==
                bool operator==(const Ch* rhs) const {return *this==GenericValue(StringRef(rhs));}
                // 当前GenericValue对象与C++11字符串直接判断是否相等的重载==
                #if RAPIDJSON_HAS_STDSTRING// 在编译器不支持C++20的三向比较运算符(<=>)时才会生效
                    bool operator==(const std::basic_string<Ch>& rhs) const {return  *this==GenericValue(StringRef(rhs));}
                #endif
                // 重载等号判断:支持与任意继承类型比较(除了指针类型和GenericValue类型,即T不能是这两种类型)
                template<typename T> RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T>>), (bool)) operator==(const T& rhs) const {return *this==GenericValue(rhs);}
                // 重载不等号
                #ifndef __cpp_impl_three_way_comparison
                    template<typename SourceAllocator>
                    // 比较当前对象和另一个GenericValue对象是否不相等
                    bool operator!=(const GenericValue<Encoding, SourceAllocator>& rhs) const {return !(*this==rhs);}
                    // 比较当前对象和另一个Ch字符串是否不相等(调用了bool operator==(const Ch* rhs))
                    bool operator!=(const Ch* rhs) const {return !(*this==rhs);}
                    // 与除了GenericValue类型的任意类型T的不相等比较
                    template<typename T> RAPIDJSON_DISABLEIF_RETURN((internal::IsGenericValue<T>), (bool)) operator!=(const T& rhs) const {return !(*this==rhs);}
                    // 定义对称的==运算符,即用于任意类型T的值在左边,需要比较是否相等的GenericValue对象在右边  注意这个==重载不是对于当前对象的比较
                    template<typename T> friend RAPIDJSON_DISABLEIF_RETURN((internal::IsGenericValue<T>), (bool)) operator==(const T& lhs, const GenericValue& rhs) {return rhs==lhs;}
                    // 定义对称的!=运算符,即用于任意类型T的值在左边,需要比较是否不相等的GenericValue对象在右边  注意这个!=重载不是对于当前对象的比较
                    template<typename T> RAPIDJSON_DISABLEIF_RETURN((internal::IsGenericValue<T>), (bool)) operator!=(const T& lhs, const GenericValue& rhs) {return !(rhs==lhs);}
                #endif
                // 获取当前GenericValue对象对应的类型
                Type GetType() const {return static_cast<Type>(data_.f.flags&kTypeMask);}
                // 判断当前对象是否是指定类型,通过标记flags来判断
                bool IsNull() const {return data_.f.flags == kNullFlag;}
                bool IsFalse() const {return data_.f.flags == kFalseFlag;}
                bool IsTrue() const {return data_.f.flags == kTrueFlag;}
                bool IsBool() const {return (data_.f.flags&kBoolFlag)!=0;}
                bool IsObject() const {return data_.f.flags == kObjectFlag;}
                bool IsArray() const {return data_.f.flags == kArrayFlag;}
                bool IsNumber() const {return (data.f.flags & kNumberFlag)!=0;}
                bool IsInt() const {return (data_.f.flags & kIntFlag)!=0;}
                bool IsUint() const {return (data_.f.flags & kUintFlag)!=0;}
                bool IsInt64() const {return (data_.f.flags & kInt64Flag)!=0;}
                bool IsUint64() const {return (data_.f.flags & kUint64Flag)!=0;}
                bool IsDouble() const {return (data_.f.flags & kDoubleFlag)!=0;}
                bool IsString() const {return (data_.f.flags & kStringFlag)!=0;}
                /**
                 * @brief 判断当前对象是否可以无损转换为双精度浮点数double
                 * 对于double类型当数据超过53位就会受损存储
                 * @return true 
                 * @return false 
                 */
                bool IsLosslessDouble() const {
                    if(!IsNumber())
                        return false;
                    if(IsUint64()){
                        uint64_t u = GetUint64();// 将当前对象转换为uint64
                        volatile double d = static_cast<double>(u);
                        return (d >= 0.0) && (d < static_cast<double>((std::numeric_limits<uint64_t>::max)())) && (u==static_cast<uint64_t>(d));
                    }
                    if(IsInt64()){
                        int64_t i = GetInt64();
                        volatile double d = static_cast<double>(i);
                        return (d < static_cast<double>((std::numeric_limits<int64_t>::max)())) && (u>=static_cast<int64_t>(d)) && (i==static_cast<int64_t>(d));
                    }
                    return true;
                }
                // 检查当前对象是否是float类型的JSON数据
                bool IsFloat() const {
                    if((data_.f.flags & kDoubleFlag)==0)
                        return false;
                    double d = GetDouble();
                    return d >= -3.4028234e38 && d <= 3.4028234e38;
                }
                // 判断当前对象是否可以无损转换为单精度浮点数float
                bool IsLosslessFloat() const {
                    if(!IsNumber())
                        return false;
                    // 为什么不先判断是否可以无损表示为double?   因为如果是有损表示为double,那么如果用这个有损的double值来判断得出可以无损转换为float,那么也说明可以无损转换为float(即使int64_t到double有损,这并不一定意味着它无法无损表示为float)
                    double a = GetDouble();// 
                    if(a < static_cast<double>(-(std::numeric_limits<float>::max)()) || a > static_cast<double>(std::numeric_limits<float>::max()))
                        return false;
                    double b = static_cast<double>(static_cast<float>(a));// double->float->double
                    return a>=b && a<=b;
                }
                /**
                 * @brief 将当前对象设置为Null值
                 * 
                 * @return GenericValue& 
                 */
                GenericValue& SetNull() {
                    this->~GenericValue();
                    new (this) GenericValue();// 默认GenericValue()构造函数就是设置的Null值
                    return *this;    
                }
                /**
                 * @brief 获取当前对象的布尔值
                 * 
                 * @return true 
                 * @return false 
                 */
                bool GetBool() const{
                    RAPIDJSON_ASSERT(IsBool());
                    return data_.f.flags == kTrueFlag;// 如果是True标记就会返回true;否则返回false
                }
                /**
                 * @brief 将当前对象设置为布尔值b
                 * 
                 * @param b 
                 * @return GenericValue& 
                 */
                GenericValue& SetBool(bool b) {
                    this->~GenericValue();
                    new (this) GenericValue(b);
                    return *this;
                }
                /**
                 * @brief 将当前对象设置为JSON对象
                 * 
                 * @return GenericValue& 
                 */
                GenericValue& SetObject() {
                    this->~GenericValue();
                    new (this) GenericValue(kObjectType);
                    return *this;
                }
                /**
                 * @brief 返回当前JSON对象的成员数
                 * 
                 * @return SizeType 
                 */
                SizeType MemberCount() const {
                    RAPIDJSON_ASSERT(IsObject());
                    return data_.o.size;
                }
                /**
                 * @brief 返回当前JSON对象的成员容量
                 * 
                 * @return SizeType 
                 */
                SizeType MemberCapacity() const {
                    RAPIDJSON_ASSERT(IsObject());
                    return data_.o.capacity;
                }
                /**
                 * @brief 判断当前JSON对象是否为空
                 * 
                 * @return true 
                 * @return false 
                 */
                bool ObjectEmpty() const {
                    RAPIDJSON_ASSERT(IsObject());
                    return data_.o.size==0;
                }
                /**
                 * @brief 通过字符串指针name来索引当前JSON对象中的成员
                 * 当去掉const后的T与Ch一样,则条件不成立.此时模板函数的返回类型为GenericValue&;若条件成立,则禁用这个模板函数
                 * internal::NotExpr:取反
                 * internal::IsSame:检查两个参数是否相同
                 * internal::RemoveConst:若T有const,则去掉T中的const
                 * @tparam T 
                 */
                template<typename T>
                RAPIDJSON_DISABLEIF_RETURN((internal::NotExpr<internal::IsSame<typename internal::RemoveConst<T>::Type, Ch>>), (GenericValue&)) operator[](T* name){
                    GenericValue n(StringRef(name));
                    return (*this)[n];// 这会调用后面的重载索引GenericValue& operator[](const GenericValue<Encoding, SourceAllocator>& name),所以若n这个key不在当前对象中,会在FindMember()中被找出
                }
                // 常量版本的索引[]
                template<typename T>
                RAPIDJSON_DISABLEIF_RETURN((internal::NotExpr<internal::IsSame<typename internal::RemoveConst<T>::Type, Ch>>), (const GenericValue&)) operator[](T* name) const {
                    return const_cast<GenericValue&>(*this)[name];// 会先调用非常量版本的[]
                }
                /**
                 * @brief 通过GenericValue表示JSON对象成员的键值来访问当前键值对的值
                 * 即类似unordered_map容器,假设一个表示JSON对象的GenericValue为a,那么a[key]就是访问对象a中匹配给定键key的键值对的对应value
                 * 其它重载的索引[],都是基于这个索引[]函数,即都调用这个
                 * @tparam SourceAllocator 
                 * @param name 
                 * @return GenericValue& 
                 */
                template<typename SourceAllocator>
                GenericValue& operator[](const GenericValue<Encoding, SourceAllocator>& name){
                    MemberIterator member = FindMember(name);// 通过name查找当前对象中是否存在以name为key的成员键值对
                    if(member != MemberEnd())// 查找到了
                        return member->value;// 返回name(键值对中的key)对应的value
                    else{// 查找不到
                        RAPIDJSON_ASSERT(false);// 调试模式(NDEBUG)才会触发断言,终止程序;发布模式下断言被禁用了,此时程序在这就不会终止
                        // 在成员未找到时,需要返回一个空的GenericValue对象,这样有助于避免未定义行为
                        #if RAPIDJSON_HAS_CXX11
                            alignas(GenericValue) thread_local static char buffer[sizeof(GenericValue)];
                            return *new (buffer) GenericValue();
                        #elif defined(__GNUC__)
                            __thread static GenericValue buffer;
                            return buffer;
                        #else
                            static GenericValue buffer;
                            return buffer;
                        #endif
                    }
                }
                // 常量版本,即返回const引用
                template<typename SourceAllocator>
                const GenericValue& operator[](const GenericValue<Encoding, SourceAllocator>& name) const {
                    return const_cast<GenericValue&>(*this)[name];// 使用const_cast的目的是能够调用前面的非常量版本的operator[]
                }
                // 使用C++11的name进行索引[]的重载版本
                #if RAPIDJSON_HAS_CXX11
                    GenericValue& operator[](const std::basic_string<Ch>& name){
                        return (*this)[GenericValue(StringRef(name))];
                    }
                    const GenericValue& operator[](const std::basic_string<Ch>& name) const {
                        return (*this)[GenericValue(StringRef(name))];
                    }
                #endif
                /**
                 * @brief 返回一个常量迭代器,指向JSON对象的第一个成员
                 * 
                 * @return ConstMemberIterator 
                 */
                ConstMemberIterator MemberBegin() const {
                    RAPIDJSON_ASSERT(IsObject());
                    return ConstMemberIterator(GetMembersPointer());
                }
                /**
                 * @brief 返回一个常量迭代器,指向当前JSON对象的最后一个成员的后一个位置
                 * 
                 * 
                 * 
                 * @return ConstMemberIterator 
                 */
                ConstMemberIterator MemberEnd() const {
                    RAPIDJSON_ASSERT(IsObject());
                    return ConstMemberIterator(GetMembersPointer()+data_.o.size);
                }
                /**
                 * @brief 返回一个可修改的迭代器(即该迭代器可以++,--等),指向当前JSON对象的第一个成员
                 * 
                 * @return MemberIterator 
                 */
                MemberIterator MemberBegin(){
                    RAPIDJSON_ASSERT(IsObject());
                    return MemberIterator(GetMembersPointer());
                }
                /**
                 * @brief 返回一个可修改的迭代器(即该迭代器可以++,--等),指向当前JSON对象的最后一个成员的后一个位置
                 * 
                 * @return MemberIterator 
                 */
                MemberIterator MemberEnd() {
                    RAPIDJSON_ASSERT(IsObject());
                    return MemberIterator(GetMembersPointer() + data_.o.size);
                }
                /**
                 * @brief 预留空间,准备为对象的成员分配新的容量
                 * 
                 * @param newCapacity 
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& MemberReserve(SizeType newCapacity, Allocator& allocator){
                    RAPIDJSON_ASSERT(IsObject());
                    DoReserveMembers(newCapacity, allocator);// 执行实际的内存分配操作,使用指定的分配器来调整成员的容量
                    return *this;
                }
                /**
                 * @brief 检查对象是否包含指定的成员(以key表示该成员)
                 * 
                 * @param name 以Ch*字符串为参数表示key
                 * @return true 
                 * @return false 
                 */
                bool HasMember(const Ch* name) const {
                    return FindMember(name) != MemberEnd();
                }
                // 以C++11字符串std::basic_string<Ch>为参数表示key
                #if RAPIDJSON_HAS_STDSTRING
                    bool HasMember(const std::basic_string<Ch>& name) const {
                        return FindMember(name) != MemberEnd();
                    }
                #endif
                /**
                 * @brief 检查对象是否包含指定的成员(以key表示该成员)
                 * 
                 * @tparam SourceAllocator 
                 * @param name 直接以const GenericValue对象为参数表示JSON对象成员的key
                 * @return true 
                 * @return false 
                 */
                template<typename SourceAllocator>
                bool HasMember(const GenericValue<Encoding, SourceAllocator>& name) const {
                    return FindMember(name) != MemberEnd();
                } 
                /**
                 * @brief 查找指定参数的成员,以迭代器返回
                 * 
                 * @param name 以Ch*字符串为参数表示key
                 * @return MemberIterator 
                 */
                MemberIterator FindMember(const Ch* name){
                    GenericValue n(StringRef(name));
                    return FindMember(n);
                }
                /**
                 * @brief 查找指定参数的成员,以常量迭代器返回
                 * 
                 * @param name 以Ch*字符串为参数表示key
                 * @return ConstMemberIterator 
                 */
                ConstMemberIterator FindMember(const Ch* name) const {
                    return const_cast<GenericValue&>(*this).FindMember(name);
                }
                /**
                 * @brief 查找指定参数的成员,以成员迭代器返回
                 * 
                 * @tparam SourceAllocator 
                 * @param name 直接以const GenericValu为参数表示key(传入的参数可以是const,也可以非const)
                 * @return MemberIterator 
                 */
                template<typename SourceAllocator>
                MemberIterator FindMember(const GenericValue<Encoding, SourceAllocator>& name){
                    RAPIDJSON_ASSERT(IsObject());
                    RAPIDJSON_ASSERT(name.IsString());
                    return DoFindMember(name);
                }
                /**
                 * @brief 查找指定参数的成员,以常量成员迭代器返回
                 * 
                 * @tparam SourceAllocator 
                 * @param name 直接以const GenericValu为参数表示key(传入的参数可以是const,也可以非const)
                 * @return ConstMemberIterator 
                 */
                template<typename SourceAllocator> 
                ConstMemberIterator FindMember(const GenericValue<Encoding, SourceAllocator>& name) const {
                    return const_cast<GenericValue&>(*this).FindMember(name);
                }
                // 参数以C++11字符串std::basic_string为例
                #if RAPIDJSON_HAS_STDSTRING
                    MemberIterator FindMember(const std::basic_string<Ch>& name) {
                        return FindMember(GenericValue(StringRef(name)));
                    }
                    ConstMemberIterator FindMember(const std::basic_string<Ch>& name) const {
                        return FindMember(GenericValue(StringRef(name)));
                    }
                #endif
                /**
                 * @brief 向当前对象添加指定的键值对成员
                 * 
                 * @param name 以GenericValue&为参数
                 * @param value 以GenericValue&为参数
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& AddMember(GenericValue& name, GenericValue& value, Allocator& allocator){
                    RAPIDJSON_ASSERT(IsObject());
                    RAPIDJSON_ASSERT(name.IsString());
                    DoAddMember(name, value, allocator);// 执行实际的添加键值对的操作,即AddMember()是DoMember()的上层封装
                    return *this;
                }
                /**
                 * @brief 向当前对象添加指定的键值对成员
                 * 
                 * @param name 以GenericValue&为参数
                 * @param value 以字符串的引用封装StringRefType为参数
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& AddMember(GenericValue& name, StringRefType value, Allocator& allocator){
                    GenericValue v(value);
                    return AddMember(name, v, allocator);
                }
                // value以C++11字符串std::basic_string为参数
                #if RAPIDJSON_HAS_STDSTRING
                    GenericValue& AddMember(GenericValue& name, std::basic_string<Ch>& value, Allocator& allocator){
                        GenericValue v(value, allocator);
                        return AddMember(name, v, allocator);
                    }
                #endif
                /**
                 * @brief 向当前对象添加指定的键值对成员
                 * 
                 * @tparam T 键值对的value以任意类型(除指针类型和GenericValue类型)为参数传入
                 */
                template<typename T>
                RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T>>), (GenericValue&)) AddMember(GenericValue& name, T value, Allocator& allocator){
                    GenericValue v(value);
                    return AddMember(name, v, allocator);
                }
                // 添加C++11中的右值引用
                #if RAPIDJSON_HAS_CXX11_RVALUE_REFS// 右值
                    GenericValue& AddMember(GenericValue&& name, GenericValue&& value, Allocator& allocator){
                        return AddMember(name, value, allocator);
                    }
                    GenericValue& AddMember(GenericValue&& name, GenericValue& value, Allocator& allocator){
                        return AddMember(name, value, allocator);
                    }
                    GenericValue& AddMember(GenericValue& name, GenericValue&& value, Allocator& allocator){
                        return AddMember(name, value, allocator);
                    }
                    GenericValue& AddMember(StringRefType name, GenericValue&& value, Allocator& allocator){
                        GenericValue n(name);
                        return AddMember(name, value, allocator);
                    }
                #endif
                /**
                 * @brief 向当前对象添加指定的键值对成员
                 * 
                 * @param name 以字符串的引用封装StringRefType为参数
                 * @param value 直接以GenericValue&为参数
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& AddMember(StringRefType name, GenericValue& value, Allocator& allocator){
                    GenericValue n(name);
                    return AddMember(n, value, allocator);
                }
                /**
                 * @brief 向当前对象添加指定的键值对成员
                 * 
                 * @param name 以字符串的引用封装StringRefType为参数
                 * @param value 以字符串的引用封装StringRefType为参数
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& AddMember(StringRefType name, StringRefType value, Allocator& allocator){
                    GenericValue v(value);
                    return AddMember(name, v, allocator);
                }
                /**
                 * @brief 以任意类型(除指针类型、GenericValue类型)为参数进行向当前对象添加指定的键值对成员
                 * 
                 * @tparam T 
                 */
                template<typename T>
                RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T>>), (GenericValue&)) AddMember(StringRefType name, T value, Allocator& allocator){
                    GenericValue n(name);
                    return AddMember(n, value, allocator);
                }
                /**
                 * @brief 类似vector的clear()方法,清空当前JSON对象中的所有成员
                 * 
                 */
                void RemoveAllMembers(){
                    RAPIDJSON_ASSERT(IsObject());
                    DoClearMembers();
                }
                /**
                 * @brief 类似C++标准库的std::remove(),都是最终直接作用于迭代器上的,即实际会调用MemberIterator RemoveMember(MemberIterator m)
                 * 
                 * @param name 以Ch*字符串为参数
                 * @return true 
                 * @return false 
                 */
                bool RemoveMember(const Ch* name){
                    GenericValue n(StringRef(name));
                    return RemoveMember(n);
                }
                // 以C++11表示的字符串std::basic_string为参数
                #if RAPIDJSON_HAS_STDSTRING
                    bool RemoveMember(const std::basic_string<Ch>& name){
                        return RemoveMember(GenericValue(StringRef(name)));
                    }
                #endif
                // 直接以GenericValue为参数
                template<typename SourceAllocator>
                bool RemoveMember(const GenericValue<Encoding, SourceAllocator>& name){
                    MemberIterator m = FindMember(name);
                    if(m!=MemberEnd()){// 可以在当前对象匹配到name表示的键值对
                        RemoveMember(m);
                        return true;
                    }
                    else
                        return false;
                }
                /**
                 * @brief 这个函数以迭代器为参数,这才是真正底层的RemoveMember(),其它几个重载RemoveMember()会调用它
                 * 
                 * @param m 
                 * @return MemberIterator 
                 */
                MemberIterator RemoveMember(MemberIterator m){
                    RAPIDJSON_ASSERT(IsObject());
                    RAPIDJSON_ASSERT(data_.o.size > 0);
                    RAPIDJSON_ASSERT(GetMembersPointer()!=0);
                    RAPIDJSON_ASSERT(m>=MemberBegin()&&m<MemberEnd());
                    return DoRemoveMember(m);// 用最后一个元素替换被删除的元素,返回指向删除位置的迭代器
                }
                /**
                 * @brief 类似vector的erase()方法(v.erase(pos);)
                 * 只传入一个参数,即只删除pos位置的JSON对象的成员
                 * @param pos 
                 * @return MemberIterator 
                 */
                MemberIterator EraseMember(ConstMemberIterator pos){
                    return EraseMember(pos, pos+1);
                }
                /**
                 * @brief 类似vector的erase()方法(v.erase(start_iterator,end_iterator);)
                 * 
                 * @param first 
                 * @param last 
                 * @return MemberIterator 
                 */
                MemberIterator EraseMember(ConstMemberIterator first, ConstMemberIterator last){
                    RAPIDJSON_ASSERT(IsObject());
                    RAPIDJSON_ASSERT(data_.o.size>0);
                    RAPIDJSON_ASSERT(GetMembersPointer()!=0);
                    RAPIDJSON_ASSERT(first>=MemberBegin());
                    RAPIDJSON_ASSERT(first<=last);
                    RAPIDJSON_ASSERT(last<=MemberEnd());
                    return DoEraseMembers(first, last);
                }
                /**
                 * @brief 删除JSON对象中的成员,通过以Ch*参数表示key
                 * 
                 * @param name 
                 * @return true 
                 * @return false 
                 */
                bool EraseMember(const Ch* name){
                    GenericValue n(StringRef(name));
                    return EraseMember(n);
                }
                // 删除JSON对象中的成员,通过以C++11字符串std::basic_string参数表示key
                #if RAPIDJSON_HAS_CXX11
                    bool EraseMember(const std::basic_string<Ch>& name){
                        return EraseMember(GenericValue(StringRef(name)));
                    }
                #endif
                /**
                 * @brief 根据给定的成员名来删除JSON对象中的成员,以GenericValue为参数表示键值对的key
                 * 
                 * @tparam SourceAllocator 
                 * @param name 
                 * @return true 
                 * @return false 
                 */
                template<typename SourceAllocator>
                bool EraseMember(const GenericValue<Encoding, SourceAllocator>& name){
                    MemberIterator m = FindMember(name);
                    if(m!=MemberEnd()){
                        EraseMember(m);// 最底层还是调用的迭代器为参数的EraseMember(),参数为ConstMemberIterator也可以接受MemberIterator,这和形参有const可以接受非const实参一样
                        return true;
                    }
                    else
                        return false;
                }
                /**
                 * @brief 将当前表示JSON对象的GenericValue对象转换为Object对象(GenericObject)
                 * 
                 * @return Object 
                 */
                Object GetObject() {
                    RAPIDJSON_ASSERT(IsObject());
                    return Object(*this);
                }   
                ConstObject GetObject() const {
                    RAPIDJSON_ASSERT(IsObject());
                    return ConstObject(*this);
                }
                /**
                 * @brief 将当前对象设置为JSON对象
                 * 
                 * @return GenericValue& 
                 */
                // 下面开始的函数是针对JSON数组的
                GenericValue& SetArray() {
                    this->~GenericValue();
                    new (this) GenericValue(kArrayType);
                    return *this;
                }
                // 返回当前JSON数组的大小
                SizeType Size() const {
                    RAPIDJSON_ASSERT(IsArray());
                    return data_.a.size;
                }
                // 返回当前JSON数组的容量
                SizeType Capacity() const {
                    RAPIDJSON_ASSERT(IsArray());
                    return data_.a.capacity;
                }
                // 判断当前JSON数组是否为空
                bool Empty() const {
                    RAPIDJSON_ASSERT(IsArray());
                    return data_.a.size == ;
                }
                // 清空当前JSON数组
                void Clear() {
                    RAPIDJSON_ASSERT(IsArray());
                    GenericValue* e = GetElementsPointer();
                    for(GenericValue* v=e;v!=e+data_.a.size;++v)
                        v->~GenericValue();
                    data.a.size = 0;
                }
                /**
                 * @brief 通过一个下标index来访问当前JSON数组对应的值
                 * 
                 * @param index 
                 * @return GenericValue& 
                 */
                GenericValue& operator[](SizeType index){
                    RAPIDJSON_ASSERT(IsArray());
                    RAPIDJSON_ASSERT(index < data_.a.size);
                    return GetElementsPointer()[index];
                }
                // 常量的索引[]版本
                const GenericValue& operator[](SizeType index) const {
                    return const_cast<GenericValue&>(*this)[index];
                }
                /**
                 * @brief 返回当前数组的首部元素的地址
                 * 
                 * @return ValueIterator 
                 */
                ValueIterator Begin() {
                    RAPIDJSON_ASSERT(IsArray());
                    return GetElementsPointer();
                }
                /**
                 * @brief 返回当前数组的尾部元素的地址
                 * 
                 * @return ValueIterator 
                 */
                ValueIterator End() {
                    RAPIDJSON_ASSERT(IsArray());
                    return GetElementsPointer()+data_.a.size;
                }
                const ValueIterator Begin() const {
                   return const_cast<GenericValue&>(*this).Begin();
                }
                const ValueIterator Begin() const {
                    return const_cast<GenericValue&>(*this).End();
                }
                /**
                 * @brief 通过给定容量大小来预留数组空间
                 * 
                 * @param newCapacity 
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& Reserve(SizeType newCapacity, Allocator& allocator){
                    RAPIDJSON_ASSERT(IsArray());// 确保是JSON数组
                    if(newCapacity > data_.a.capacity){// 如果新设容量不比之前大,就不重新设置了,免得多次分配内存,造成性能损失
                        SetElementsPointer(reinterpret_cast<GenericValue*>(allocator.Realloc(GetElementsPointer(), data.a.capacity*sizeof(GenericValue), newCapacity*sizeof(GenericValue)));
                        data_.a.capacity = newCapacity;
                    }
                    return *this;
                }
                /**
                 * @brief 向当前JSON数组中压入一个元素
                 * 
                 * @param value 
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& PushBack(GenericValue& value, Allocator& allocator){
                    RAPIDJSON_ASSERT(IsArray());// 确保是JSON数组
                    if(data_.a.size >= data_.a.capacity)// 如果数组的当前大小已经达到或超过容量,则调用Reserve方法重新分配内存,以容纳更多元素
                        Reserve(data_.a.capacity==0 ? kDefaultArrayCapacity:(data_.a.capacity+(data_.a.capacity+1)/2), allocator);// 使用当前容量的加倍策略来增加容量
                    GetElementsPointer()[data_.a.size++].RawAssign(value);// 将value插入到数组的最后一个位置
                    return *this;
                }
                // 定义一个接收右值引用的PushBack方法
                #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
                    GenericValue& PushBack(GenericValue&& value, Allocator& allocator){
                        return PushBack(value, allocator);// 这里使用了C++11的引用折叠
                    }
                #endif
                /**
                 * @brief 用于将一个StringRefType类型的值推送到数组中
                 * 
                 * @param value 
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& PushBack(StringRefType value, Allocator& allocator){
                    return (*this).template PushBack<StringRefType>(value, allocator);
                }
                /**
                 * @brief 它接受一个任意类型T的值,如果T既不是指针也不是GenericValue类型,则将value转换为GenericValue类型并调用PushBack插入到数组中
                 * 
                 * @tparam T 
                 */
                template<typename T>
                RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T>>), (GenericValue&)) PushBack(T value, Allocator& allocator){
                    GenericValue v(value);
                    return PushBack(v, allocator);
                }
                /**
                 * @brief 从当前JSON数组的末尾移除一个元素
                 * 
                 * @return GenericValue& 
                 */
                GenericValue& PopBack(){
                    RAPIDJSON_ASSERT(IsArray());// 确保是JSON数组
                    RAPIDJSON_ASSERT(!Empty());
                    GetElementsPointer()[--data_.a.size].~GenericValue();
                    return *this;
                }
                /**
                 * @brief 用于删除当前JSON数组中指定位置的元素
                 * 
                 * @param pos 
                 * @return ValueIterator 
                 */
                ValueIterator Erase(ConstValueIterator pos){
                    return Erase(pos, pos+1);
                }
                /**
                 * @brief 删除当前JSON数组中指定范围的元素(first到last).返回新的位置迭代器
                 * 
                 * 
                 * @param first 
                 * @param last 
                 * @return ValueIterator 
                 */
                ValueIterator Erase(ConstValueIterator first, ConstValueIterator last){
                    RAPIDJSON_ASSERT(IsArray());// 确保是JSON数组
                    RAPIDJSON_ASSERT(data_.a.size>0);
                    RAPIDJSON_ASSERT(GetElementsPointer()!=0);
                    RAPIDJSON_ASSERT(first >= Begin());
                    RAPIDJSON_ASSERT(first <= last);
                    RAPIDJSON_ASSERT(last <= End());
                    ValueIterator pos = Begin()+(first-Begin());
                    for(ValueIterator itr=pos;itr!=last;++itr)
                        itr->~GenericValue();
                    std::memmove(static_cast<void*>(pos), last, static_cast<size_t>(End()-last)*sizeof(GenericValue));// 将last后续未被删除的移到pos(即前面删除的起始位置)
                    data_.a.size -= static_cast<SizeType>(last-first);
                    return pos;
                }
                /**
                 * @brief 将当前表示JSON数组的GenericValue对象转换为Array对象(GenericArray)
                 * 
                 * @return Array 
                 */
                Array GetArray() {
                    RAPIDJSON_ASSERT(IsArray());
                    return Array(*this);
                }
                ConstArray GetArray() const {
                    RAPIDJSON_ASSERT(IsArray());
                    return ConstArray(*this);
                }
                /**
                 * @brief 返回当前的对象的整数值
                 * 
                 * @return int 
                 */
                int GetInt() const {
                    RAPIDJSON_ASSERT(data_.f.flags&kIntFlag);
                    return data_.n.i.i;
                }
                /**
                 * @brief 返回当前的对象的无符号整数值
                 * 
                 * @return unsigned 
                 */
                unsigned GetUint() const {
                    RAPIDJSON_ASSERT(data_.f.flags&kUintFlag);
                    return data_.n.u.u;
                }
                /**
                 * @brief 返回当前的对象的64位整数值
                 * 
                 * @return int64_t 
                 */
                int64_t GetInt64() const {
                    RAPIDJSON_ASSERT(data_.f.flags&kInt64Flag);
                    return data_.n.i64;
                }
                /**
                 * @brief 返回当前的对象的无符号64位整数值
                 * 
                 * @return uint64_t 
                 */
                uint64_t GetUint64() const {
                    RAPIDJSON_ASSERT(data_.f.flags&kUint64Flag);
                    return data_.n.u64;
                }
                /**
                 * @brief 返回当前的对象的双精度浮点值
                 * 
                 * @return double 
                 */
                double GetDouble() const {
                    RAPIDJSON_ASSERT(IsNumber());
                    if((data_.f.flags&kDoubleFlag)!=0)// 如果当前对象为双精度值,则直接返回
                        return data_n.d;
                    if((data_.f.flags&kIntFlag)!=0)// 如果当前对象是整数值,则以双精度形式返回,因为int转换为double是安全的隐式转换,所以不用static_cast<double>
                        return data_n.i.i;
                    if((data_.f.flags&kUintFlag)!=0)// 如果当前对象是无符号整数值,则以双精度形式返回,因为Uint转换为double是安全的隐式转换,所以不用static_cast<double>
                        return data_n.u.u;
                    if((data_.f.flags&kInt64Flag)!=0)// 如果当前对象是64位整数值,则以双精度形式返回,因为int64转换为double是可能会出现精度损失的情况,所以需要static_cast<double>
                        return static_cast<double>(data_.n.i64);
                    RAPIDJSON_ASSERT((data_.f.flags&kUint64Flag)!=0);// 剩余情况只能是无符号64位整数了
                    return static_cast<double>(data_.n.u64);
                }
                /**
                 * @brief 将当前GenericValue对象的值转换为float类型并返回
                 * 
                 * @return float 
                 */
                float GetFloat() const {
                    return static_cast<float>(GetDouble());
                }
                /**
                 * @brief 将当前GenericValue对象的值设置为int类型
                 * 
                 * @param i 
                 * @return GenericValue& 
                 */
                GenericValue& SetInt(int i){
                    this->~GenericValue();
                    new (this) GenericValue(i);
                    return *this;
                }
                // 将当前GenericValue对象的值设置为Uint类型
                GenericValue& SetUint(unsigned u){
                    this->~GenericValue();
                    new (this) GenericValue(u);
                    return *this;
                }
                // 将当前GenericValue对象的值设置为int64类型
                GenericValue& SetInt64(int64_t i64){
                    this->~GenericValue();
                    new (this) GenericValue(i64);
                    return *this;
                }
                // 将当前GenericValue对象的值设置为uint64类型
                GenericValue& SetUint64(uint64_t u64){
                    this->~GenericValue();
                    new (this) GenericValue(u64);
                    return *this;
                }
                // 将当前GenericValue对象的值设置为double类型
                GenericValue& SetDouble(double d){
                    this->~GenericValue();
                    new (this) GenericValue(d);
                    return *this;
                }
                // 将当前GenericValue对象的值设置为float类型
                GenericValue& SetFloat(float f){
                    this->~GenericValue();
                    new (this) GenericValue(static_cast<float>(f));
                    return *this;
                }
                /**
                 * @brief 返回当前GenericValue对象所存储的字符串内容,按Ch*的形式返回
                 * 
                 * @return const Ch* 
                 */
                const Ch* GetString() const {
                    RAPIDJSON_ASSERT(IsString());
                    return DataString(data_);// 返回指向字符串数据的指针
                }
                /**
                 * @brief 返回当前GenericValue对象所存储字符串的长度
                 * 
                 * @return SizeType 
                 */
                SizeType GetStringLength() const {
                    RAPIDJSON_ASSERT(IsString());
                    return DataStringLength(data_);// 获取字符串长度
                }
                /**
                 * @brief 将当前GenericValue对象的值data_设置为字符串类型,值为指向s的指针,长度为length
                 * s以Ch*为参数传入
                 * @param s 
                 * @param length 
                 * @return GenericValue& 
                 */
                GenericValue& SetString(const Ch* s, SizeType length){
                    return SetString(StringRef(s, length));
                }
                /**
                 * @brief 将当前GenericValue对象的值data_设置为s
                 * s以StringRefType为参数传入
                 * @param s 
                 * @return GenericValue& 
                 */
                GenericValue& SetString(StringRefType s){
                    this->~GenericValue();
                    SetStringRaw(s);// 将当前GenericValue对象的data_值设置为常量字符串s,并标记该GenericValue对象的flags=kConstStringFlag
                    return *this;
                }
                /**
                 * @brief 将当前GenericValue对象的data_值设置为s,并使用Allocator分配内存
                 * s以Ch*为参数传入
                 * @param s 
                 * @param length 
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& SetString(const Ch* s, SizeType length, Allocator& allocator){
                    return SetString(StringRef(s), allocator);
                }
                /**
                 * @brief 将当前GenericValue对象的data_值设置为s,并使用Allocator分配内存
                 * s以StringRefType为参数传入
                 * @param s 
                 * @param allocator 
                 * @return GenericValue& 
                 */
                GenericValue& SetString(StringRefType s, Allocator& allocator){
                    this->~GenericValue();
                    SetStringRaw(s, allocator);// 通过内存分配器复制字符串的方法将s复制到当前GenericValue对象中的data_中
                    return *this;
                }
                // 以C++11字符串std::basic_string为参数传入
                #if RAPIDJSON_HAS_CXX11
                    GenericValue& SetString(const std::basic_string<Ch>& s, Allocator& allocator){
                        return SetString(StringRef(s), allocator);
                    }
                #endif
                // 查找当前GenericValue对象是否为T类型
                template<typename T>
                bool Is() const {
                    return internal::TypeHelper<ValueType, T>::Is(*this);
                }
                // 返回当前GenericValue对象中T类型的值
                template<T>
                T Get() const {
                    return internal::TypeHelper<ValueType, T>::Get(*this);
                }
                template<typename T>
                T Get() {
                    return internal::TypeHelper<ValueType, T>::Get(*this);
                }
                // 设置当前GenericValue对象的data_为T类型,并等于data
                template<typename T>
                ValueType& Set(const T& data){
                    return internal::TypeHelper<ValueType, T>::Set(*this, data);
                }
                template<typename T>
                ValueType& Set(const T& data, Allocator& allocator){
                    return internal::TypeHelper<ValueType, T>::Set(*this, data, allocator);
                }
                /**
                 * @brief 接受并处理传入的Handler对象,它对当前GenericValue对象进行遍历并根据不同的数据类型执行指定的Handler操作
                 * Handler通常是用户自定义从外部传入的
                 * @tparam Handler 
                 * @param handler 
                 * @return true 
                 * @return false 
                 */
                template<typename Handler>
                bool Accept(Handler& handler) const {
                    switch(GetType()){
                        case kNullType:// Handler处理Null类型
                            return handler.Null();
                        case kFalseType:// Handler处理False类型
                            return handler.Bool(false);
                        case kTrueType:// // Handler处理true类型
                            return handler.Bool(true);
                        case kObjectType:// // Handler处理Object类型
                            if(RAPIDJSON_UNLIKELY(!handler.StartObject()))
                                return false;
                            for(ConstMemberIterator m=MemberBegin();m!=MemberEnd();++m){// 遍历JSON对象的每个键值对
                                RAPIDJSON_ASSERT(m->name.IsString());// JSON对象的键值对的键必须是字符串
                                if(RAPIDJSON_UNLIKELY(!handler.Key(m->name.GetString(), m->name.GetStringLength(), (m->name.data_.f.flags&kCopyFlag)!=0)))
                                    return false;
                                if(RAPIDJSON_UNLIKELY(!m->value.Accept(handler)))// JSON对象的键值对的值可以是任何值类型,所以这里还需单独对m->value用handler处理
                                    return false;
                            }
                            return handler.EndObject(data_.o.size);
                        case kArrayType:// Handler处理Array类型
                            if(RAPIDJSON_UNLIKELY(!handler.StartArray()))
                                return false;
                            for(ConstValueIterator v=Begin();v!=End();++v)// 遍历JSON数组的每个元素
                                if(RAPIDJSON_UNLIKELY(!v->Accept(handler)))
                                    return false;
                            return handler.EndArray(data_.a.size);
                        case kStringType:// Handler处理String类型
                            return handler.String(GetString(), GetStringLength(), (data_.f.flags&kCopyFlag)!=0);
                        default:// 剩余情况只能是数值类型
                            RAPIDJSON_ASSERT(GetType()==kNumberType);
                            if(IsDouble())
                                return handler.Double(data_.n.d);
                            else if(IsInt())
                                return handler.Int(data_.n.i.i);
                            else if(IsUint())
                                return handler.Uint(data_.n.u.u);
                            else if(IsInt64())
                                return handler.Int64(data_.n.ui64);
                            else
                                return handler.Uint64(data_.n.u64);
                    }
                }
            private:
                template<typename, typename> friend class GenericValue;// 声明GenericValue是当前类的友元类,即它可以访问当前类的私有成员
                template<typename, typename> friend class GenericDocument;// 声明GenericDocument是当前类的友元类,即它可以访问当前类的私有成员
                enum {
                    kBoolFlag = 0x0008,
                    kNumberFlag = 0x0010,
                    kIntFlag = 0x0020,
                    kUintFlag = 0x0040,
                    kInt64Flag = 0x0080,
                    kUint64Flag = 0x0100,
                    kDoubleFlag = 0x0200,
                    kStringFlag = 0x0400,
                    kCopyFlag = 0x0800,
                    kInlineStrFlag = 0x1000,// 标识内联字符串标志,通常用于标识字符串对象内联存储.ShortString就是内联存储的,即它的字符串直接存储在这个结构体内存中,而不是像String中那样字符串的存储位置用一个指向其他内存位置的指针来表示
                    kNullFlag = kNullType,
                    // 定义复合标志
                    kTrueFlag = static_cast<int>(kTrueType) | static_cast<int>(kBoolFlag);
                    kFalseFlag = static_cast<int>(kFalseType) | static_cast<int>(kBoolFlag);
                    kNumberIntFlag = static_cast<int>(kNumberType) | static_cast<int>(kNumberType | kIntFlag | kInt64Flag);
                    kNumberUintFlag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kUintFlag | kUint64Flag),
                    kNumberInt64Flag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kInt64Flag),
                    kNumberUint64Flag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kUint64Flag),
                    kNumberDoubleFlag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kDoubleFlag),
                    kNumberAnyFlag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kIntFlag | kInt64Flag | kUintFlag | kUint64Flag | kDoubleFlag),
                    kConstStringFlag = static_cast<int>(kStringType) | static_cast<int>(kStringFlag),
                    kCopyStringFlag = static_cast<int>(kStringType) | static_cast<int>(kStringFlag | kCopyFlag),
                    kShortStringFlag = static_cast<int>(kStringType) | static_cast<int>(kStringFlag | kCopyFlag | kInlineStrFlag),
                    kObjectFlag = kObjectType,
                    kArrayFlag = kArrayType,
                    kTypeMask = 0x07// 用于提取类型信息的掩码,通过最低3位来标识数据的基本类型
                };
                static const SizeType kDefaultArrayCapacity = RAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY;
                static const SizeType kDefaultObjectCapacity = RAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY;
                /**
                 * @brief Flag结构体用于存储类型标志信息,根据不同的优化选项,该结构体会使用不同大小的内存布局,以确保内存对齐和节省空间
                 * payload中后续加的数字是用于对齐的,如:48位时+6达到14字节,此时这个结构体就是16字节;64位时+6达到22字节,此时这个结构体就是24字节
                 * 
                 */
                struct Flag{
                    #if RAPIDJSON_48BITPOINTER_OPTIMIZATION
                        char payload[sizeof(SizeType)*2+6];// 14字节=2*4+6 6个填充字节确保内存对齐和高效访问
                    #elif RAPIDJSON_64BIT
                        char payload[sizeof(SizeType)*2+sizeof(void*)+6];// 22字节=2*4+8+6
                    #else
                        char payload[sizeof(SizeType)*2+sizeof(void*)+2]// 14字节=2*4+4+2
                    #endif
                        uint46_t flags;// 2字节
                };
                /**
                 * @brief 用于存储字符串数据
                 * 
                 */
                struct String{
                    SizeType length;
                    SizeType hashcode;// 为字符串预留的哈希码,通常用于优化查找
                    const Ch* str;
                };
                /**
                 * @brief 用于存储较短的字符串.使用了最后一个有效字节存储反向字符串长度的方法
                 * RAPIDJSON_48BITPOINTER_OPTIMIZATION启用时ShortString有13字节的字符串存储位置;64位系统中有21字节;32位系统中有13字节
                 */
                struct ShortString{
                    enum {
                        MaxChars = sizeof(static_cast<Flag*>(0)->payload)/sizeof(Ch),
                        MaxSize = MaxChars-1,
                        LenPos = MaxSize// std[LenPos]存储该字符串的长度信息
                    };
                    Ch str[MaxChars];// 这是内联存储,即存储在结构体这个内存中
                    inline static bool Usable(SizeType len) {// 判断给定的长度是否小于或等于最大有效字符数,即在32位系统中小于13字节的字符串都是ShortString
                        return (MaxSize >= len);
                    }
                    inline void SetLength(SizeType len) {// 在最后一个字节设置字符串的长度信息,注意:这里用的是反向方式存储,不是直接存储该字符串的实际长度
                        str[LenPos] = static_cast<Ch>(MaxSize-len);
                    }
                    inline SizeType GetLength() const {// 获取该字符串实际长度
                        return static_cast<SizeType>(MaxSize - str[LenPos]);
                    }
                };
                /**
                 * @brief 用于存储不同类型的数字数据
                 * 使用了padding数组进行对齐填充,使每个数据都是8字节
                 */
                union Number{
                    #if RAPIDJSON_ENDIAN == RAPIDJSON_LITTLEENDIAN// 小端序
                        struct I {
                            int i;
                            char padding[4];// 后面填充字节,使其都为8字节,进行对齐
                        }i;
                        struct U {
                            unsigned u;
                            char padding2[4];
                        }u;
                    #else
                        struct I {
                            char padding[4];// 前面填充字节,使其都为8字节,进行对齐
                            int i;
                        }i;
                        struct U {
                            char padding2[4];
                            unsigned u;
                        }u;
                    #endif
                    int64_t i64;
                    uint64_t u64;
                    double d;
                };
                /**
                 * @brief 用于存储JSON对象数据
                 * 
                 */
                struct ObjectData {
                    SizeType size;
                    SizeType capacity;
                    Member* members;
                };
                /**
                 * @brief 用于存储JSON数组数据
                 * 
                 */
                struct ArrayData {
                    SizeType size;
                    SizeType capacity;
                    GenericValue* elements;
                };
                /**
                 * @brief 在同一内存位置存储不同类型的数据,使用联合体可以节省内存
                 * 
                 */
                union Data {
                    String s;
                    ShortString ss;
                    Number n;
                    ObjectData o;
                    ArrayData a;
                    Flag f;
                };
                // 返回当前JSON数据中存储的字符串内容.kInlineStrFlag=>ShortString
                static RAPIDJSON_FORCEINLINE const Ch* DataString(const Data&){
                    return (data.f.flags & kInlineStrFlag) ? data.ss.str : RAPIDJSON_GETPOINTER(Ch, data.s.str);
                }
                // 返回当前JSON数据中存储的字符串长度
                static RAPIDJSON_FORCEINLINE SizeType DataStringLength(const Data& data){
                    return (data.f.flags & kInlineStrFlag) ? data.ss.GetLength() : data.s.length;
                }
                // 以下几个函数都是操作的当前JSON对象/数组/字符串
                // 返回当前JSON字符串的指针.简单情况就是直接返回data_.s.str
                RAPIDJSON_FORCEINLINE const Ch* GetStringPointer() const {
                    return RAPIDJSON_GETPOINTER(Ch, data_.s.str);
                }
                // 设置当前JSON字符串的指针.即让data_.s.str指向str相同的地方
                RAPIDJSON_FORCEINLINE const Ch* SetStringPointer(const Ch* str) {
                    return RAPIDJSON_SETPOINTER(Ch, data_.s.str, str);
                }
                // 获取当前JSON数组元素的指针.简单情况就是直接返回data_.a.elements
                RAPIDJSON_FORCEINLINE GenericValue* GetElementsPointer() const {
                    return RAPIDJSON_GETPOINTER(GenericValue, data_.a.elements);
                }
                // 设置当前JSON数组元素的指针.即让data_.a.elements指向elements相同的地方
                RAPIDJSON_FORCEINLINE GenericValue* SetElementsPointer(GenericValue* elements) {
                    return RAPIDJSON_SETPOINTER(GenericValue, data_.a.elements, elements);
                }
                // 获取当前JSON对象成员的指针.简单情况就是直接返回data_.o.members
                RAPIDJSON_FORCEINLINE Member* GetMembersPointer() const {
                    return RAPIDJSON_GETPOINTER(Member, data_.o.members);
                }
                // 设置当前JSON对象成员的指针.即让data_.a.members指向members相同的地方
                RAPIDJSON_FORCEINLINE Member* SetMembersPointer(Member* members) {
                    return RAPIDJSON_SETPOINTER(Member, data_.o.members, members);
                }
                #if RAPIDJSON_USE_MEMBERSMAP// 如果启用成员映射功能(JSON对象才有成员)
                    struct MapTraits{
                        struct Less{// 使用函数对象自定义比较函数   用于比较两个Data类型的数据
                            bool operator()(const Data& s1, const Data& s2) const {// operator重载函数调用运算符
                                SizeType n1 = DataStringLength(s1), n2 = DataStringLength(s2);
                                int cmp = std::memcmp(DataString(s1), DataString(s2), sizeof(Ch)*(n1<n2 ? n1:n2));// 只比较s1和s2两个字符串最少的个数,如:Length(s1)=3,Length(s2)=5,那么只比较3个字符
                                return cmp<0 || (cmp==0&&n1<n2);// 如果cmp小于0,表示s1<s2;如果cmp=0,则比较字符串长度
                            }
                        };
                        typedef std::pair<const Data, SizeType> Pair;
                        typedef std::multimap<Data, SizeType, Less, StdAllocator<Pair, Allocator>> Map;// key=Data, value=SizeType;Map中存储的是JSON对象成员的键(key)与该成员在对象中的位置索引,而不是JSON键与JSON值的映射
                        typedef typename Map::iterator Iterator;// 定义Iterator类型为Map类型的迭代器
                    };
                    typedef typename MapTraits::Map         Map;
                    typedef typename MapTraits::Less        MapLess;
                    typedef typename MapTraits::Pair        MapPair;
                    typedef typename MapTraits::Iterator    MapIterator;
                    // 计算和返回存储映射所需的内存布局的总大小
                    // 在内存布局中我们自定义成了如下形式:Map指针内存+SizeType容量内存+capacity*sizeof(Member)所有成员内存+capacity * sizeof(MapIterator)所有映射的迭代器内存
                    static RAPIDJSON_FORCEINLINE size_t GetMapLayoutSize(SizeType capacity) {
                        return  RAPIDJSON_ALIGN(sizeof(Map*)) +
                                RAPIDJSON_ALIGN(sizeof(SizeType)) + 
                                RAPIDJSON_ALIGN(capacity * sizeof(Member)) + 
                                capacity * sizeof(MapIterator);
                    }
                    // 通过映射获取容量,通过此函数确保了SizeType表示的容量在Map*之后
                    static RAPIDJSON_FORCEINLINE SizeType& GetMapCapacity(Map* &map) {// 指针的引用
                        return *reinterpret_cast<SizeType*>(reinterpret_cast<uintptr_t>(&map) + RAPIDJSON_ALIGN(sizeof(Map*)));
                    }
                    // 通过映射来获取给定map的成员地址,通过此函数确保了在内存布局中:Member在Map指针内存+SizeType容量内存的后面,并且得到的Member*就是传入的map的键值对指针
                    static RAPIDJSON_FORCEINLINE Member* GetMapMembers(Map* &map){
                        return reinterpret_cast<Member*>(reinterpret_cast<uintptr_t>(&map) + 
                                                         RAPIDJSON_ALIGN(sizeof(Map*)) + 
                                                         RAPIDJSON_ALIGN(sizeof(SizeType)));
                    }
                    // 通过映射来获取成员迭代器在内存中要存储的地址,通过此函数确保了在内存布局中:MapIterator在Map指针内存+SizeType容量内存+Member内存的后面,,并且得到的MapIterator*就是传入的map的键值对的迭代器
                    static RAPIDJSON_FORCEINLINE MapIterator* GetMapIterators(Map* &map){
                        return reinterpret_cast<MapIterator*>(reinterpret_cast<uintptr_t>(&map) + 
                                                         RAPIDJSON_ALIGN(sizeof(Map*)) + 
                                                         RAPIDJSON_ALIGN(sizeof(SizeType)) + 
                                                         RAPIDJSON_ALIGN(GetMapCapacity(map) * sizeof(Member)));
                    }
                    /**
                     * @brief 通过给定的Member* members指针,反向查找Map*指针的位置并返回对其的引用
                     * 
                     * @param members 
                     * @return RAPIDJSON_FORCEINLINE*& 
                     */
                    static RAPIDJSON_FORCEINLINE Map* &GetMap(Member* members){
                        RAPIDJSON_ASSERT(members!=0);
                        return *reinterpret_cast<Map**>(reinterpret_cast<uintptr_t>(members) - 
                                                        RAPIDJSON_ALIGN(sizeof(SizeType)) - 
                                                        RAPIDJSON_ALIGN(sizeof(Map*)));
                    }
                    // 析构传入的MapIterator迭代器并返回新的迭代器,新的迭代器和传入的是一样的.这样做可以返回一个安全的迭代器,即这个迭代器当前是没有被外部持有的,而rhs这个迭代器可能在外部某个地方被持有了
                    RAPIDJSON_FORCEINLINE MapIterator DropMapIterator(MapIterator& rhs) {
                        #if RAPIDJSON_HAS_CXX11
                            MapIterator ret = std::move(rhs);
                        #else
                            MapIterator ret = rhs;
                        #endif
                            rhs.~MapIterator();
                            return ret;
                    }
                    /**
                     * @brief 对Map数据结构重新分配内存并将旧的数据迁移到新分配的内存中
                     * 
                     * @param oldMap 旧指针的指针
                     * @param newCapacity 
                     * @param allocator 
                     * @return Map*& 
                     */
                    Map* &DoReallocMap(Map** oldMap, sizeType newCapacity, Allocator& allocator) {
                        Map** newMap = static_cast<Map**>(allocator.Malloc(GetMapLayoutSize(newCapacity)));// 分配按[ Map* ] [ SizeType ] [ Member[capacity] ] [ MapIterator[capacity] ]这种内存形式布局的这么大的内存空间
                        GetMapCapacity(*neMap) = newCapacity;// 找到容量地址并重新设置新容量
                        if(!oldMap)// 无旧Map,则new一个
                            *newMap = new (allocator.Malloc(sizeof(Map))) Map(MapLess(), allocator);// 分配一个新的Map对象
                        else{// 有旧Map,则将旧数据迁移到新分配的内存中
                            *newMap = *oldMap;
                            size_t count = (*oldMap)->size();// 旧Map的大小
                            std::memcpy(static_cast<void*>(GetMapMembers(*newMap)),
                                        static_cast<void*>(GetMapMembers(*oldMap)),
                                        count * sizeof(Member));// 将旧Map中的成员从旧Map内存拷贝到新的Map中
                            MapIterator *oldIt = GetMapIterator(*oldMap),// 旧Map的迭代器数组
                                        *newIt = GetMapIterator(*newMap);// 新Map的迭代器数组
                            while(count--)// 释放旧Map的内存
                                new (&newIt[count]) MapIterator(DropMapIterator(oldIt[count]));// 复制到newIt迭代器
                            Allocator::Free(oldMap);
                        }
                        return *newMap;// 返回新Map
                    }
                    // 分配一块内存来存储capacity个成员,并返回分配的Member数组的指针
                    RAPIDJSON_FORCEINLINE Member* DoAllocMembers(SizeType capacity, Allocator& allocator) {
                        return GetMapMembers(DoReallocMap(0, capacity, allocator));
                    }
                    // 通过增加容量来预留空间,如果newCapacity大于当前容量,则会重新分配内存
                    void DoReserveMembers(SizeType newCapacity, Allocator& allocator) {
                        ObjectData& o = data_.o;
                        if(newCapacity > o.capacity){// 如果新分配的容量更大,那么就要重新分配内存
                            Member* oldMembers = GetMembersPointer();
                            Map** oldMap = oldMembers ? &GetMap(oldMembers):nullptr,// oldMembers有成员则返回旧的map指针,否则返回nullptr
                               *& newMap = DoReallocMap(oldMap, newCapacity, allocator);// 重新分配Map内存
                            RAPIDJSON_SETPOINTER(Member, o.members, GetMapMembers(newMap));// 将新的成员地址赋给o.members
                            o.capacity = newCapacity;
                        }
                    }
                    /**
                     * @brief 查找指定名称的成员,并返回该成员的迭代器;找不到成员,就返回MemberEnd()
                     * 
                     * @tparam SourceAllocator 
                     * @param name 
                     * @return MemberIterator 
                     */
                    template<typename SourceAllocator>
                    MemberIterator DoFindMember(const GenericValue<Encoding, SourceAllocator>& name) {
                        if(Member* members=GetMembersPointer()){
                            Map*& map = GetMap(members);
                            MapIterator mit = map->find(reinterpret_cast<const Data&*>(name.data_));
                            if(mit!=map->end())// 找到了
                                return MemberIterator(&members[mit->second]);// 返回其成员迭代器
                        }
                        return MemberEnd();// 找不到就直接返回最后一个成员的后一个位置
                    }
                    /**
                     * @brief 清空所有成员,将Map中的成员删除,即将JSON对象的成员删除
                     * 清空成员,没有删除成员数组
                     */
                    void DoClearMembers() {
                        if(Member* members = GetMembersPointer()){// 获取当前成员数组的指针
                            Map*& map = GetMap(members);// 获取与members相关的Map
                            MapIterator* mit = GetMapIterator(map);// 获取指向Map的迭代器
                            for(SizeType i=0;i<data_.o.size;++i){
                                map->erase(DropMapIterator(mit[i]));// 从map中移除成员
                                members[i].~Member();// 销毁成员对象
                            }
                            data_.o.size = 0;
                        }
                    }
                    /**
                     * @brief 释放Map和成员对象内存
                     * 删除了删除数组,并且释放了内存
                     */
                    void DoFreeMembers() {
                        if(Member* members=GetMembersPointer()) {
                            GetMap(members)->~Map();
                            for(SizeType i=0;i<data_.o.size;++i)
                                members[i].~Member();
                            if(Allocator::kNeedFree){
                                Map** map = &GetMap(members);
                                Allocator::Free(*map);
                                Allocator::Free(map);
                            }
                        }
                    }
                #else// 不用map
                    // 用于为Member[]数组分配内存(即为JSON对象的成员数组分配空间)
                    RAPIDJSON_FORCEINLINE Member* DoAllocMembers(SizeType capacity, Allocator& allocator) {
                        return Malloc<Member>(allocator, capacity);
                    }
                    // 用于调整Member[]数组的容量
                    void DoReserveMembers(SizeType newCapacity, Allocator& allocator) {
                        ObjectData& o = data_.o;
                        if(newCapacity > o.capacity) {// 如果新分配的容量更大,那么就要重新分配内存
                            Member* newMembers = Realloc<Member>(allocator, GetMembersPointer(), o.capacity, newCapacity);
                            RAPIDJSON_SETPOINTER(Member, o.members, newMembers);
                            o.capacity = newCapacity;
                        }
                    }
                    // 用于查找特定名称的成员在Member[]数组中的位置,以迭代器返回
                    template<typename SourceAllocator>
                    MemberIterator DoFindMember(const GenericValue<Encoding, SourceAllocator>& name) {
                        MemberIterator member = MemberBegin();
                        for(;member!=MemberEnd();++member)
                            if(name.StringEqual(member->name))// 查找到了,直接退出循环
                                break;
                        return member;
                    }
                    // 清空当前JSON对象的Member[]
                    void DoClearMembers() {
                        for(for(MemberIterator m=MemberBegin();m!=MemberEnd();++m)
                            m->~Member();
                        Allocator::Free(GetMembersPointer());
                    }
                #endif// RAPIDJSON_USE_MEMBERSMAP
                /**
                 * @brief 用于向当前JSON对象添加一个新的成员  name:value
                 * 
                 * @param name 
                 * @param value 
                 * @param allocator 
                 */
                void DoAddMember(GenericValue& name, GenericValue& value, Allocator& allocator) {
                    ObjectData& o = data_.o;
                    if(o.size >= o.capacity)// 检查当前容量是否满了,即是否需要扩容后再添加成员
                        DoReserveMembers(o.capacity ? (o.capacity + (o.capacity+1)/2:kDefaultObjectCapacity, allocator));// 使用当前容量的加倍策略来增加容量
                    Member* members = GetMembersPointer();// 获取指向成员数组的指针
                    Member* m = members+o.size;// 将m指向成员数组的下一个空位置,为了后续将新键值对添加进来
                    m->name.RawAssign(name);// name赋值给新键值对的name成员(即赋给m这个Member对象中的name成员)
                    m->value.RawAssign(value);// value赋值给新键值对的value成员(即赋给m这个Member对象中的value成员)
                    #if RAPIDJSON_USE_MEMBERSMAP
                        Map*& map = GetMap(members);// 获取map这个映射表的地址
                        MapIterator* mit = GetMapIterator(map);// 获取map的迭代器指针
                        new (&mit[o.size]) MapIterator(map->insert(MapPair(m->name.data_, o.size)));// 将name和其索引位置添加到map这个映射中
                    #endif
                    ++o.size;
                }
                /**
                 * @brief 用于从JSON对象中删除一个指定的成员
                 * 
                 * @param m 待删除的成员的迭代器
                 * @return MemberIterator 
                 */
                MemberIterator DoRemoveMember(MemberIterator m) {
                    ObjectData& o = data_.o;
                    Member* members = GetMembersPointer();// 获取当前JSON对象的成员数组的指针
                    #if RAPIDJSON_USE_MEMBERSMAP// 启用了map映射,则在映射表中删除对应的成员
                        Map*& map = GetMap(members);
                        MapIterator* mit = GetMapIterator(map);
                        SizeType mpos = static_cast<SizeType>(&*m-members);
                        map->erase(DropMapIterator(mit[mpos]));// 从映射表(std::multimap)中删除对应成员mit[mpos]
                    #endif
                    MemberIterator last(members+(o.size-1));// 获取成员数组中最后一个成员的迭代器
                    if(o.size>1 && m!=last) {// 如果成员数组中有多个成员,且要删除的成员不是最后一个,则将最后一个成员复制到被删除的成员的位置
                        #if RAPIDJSON_USE_MEMBERSMAP
                            new (&mit[mpos]) MapIterator(DropMapIterator(mit[&*last-members]));// 将最后一个成员复制到mit[mpos]这个需要删除成员的位置
                            mit[mpos]->second = mpos;
                        #endif
                        *m = *last;// 将最后一个成员的内容复制到要删除的成员位置,这样既实现了删除指定成员,又把最后一个成员复制过来了
                    }
                    else
                        m->~Member();
                    --o.size;
                    return m;
                }
                /**
                 * @brief 用于从JSON对象中删除一段连续的成员 first到last
                 * 
                 * @param first 
                 * @param last 
                 * @return MemberIterator 
                 */
                MemberIterator DoEraseMembers(ConstMemberIterator first, ConstMemberIterator last) {
                    ObjectData& o = data_.o;
                    MemberIterator beg = MemberBegin(),
                                   pos = beg+(first-beg),// 待删除成员的起点,即first
                                   end = MemberEnd();
                    #if RAPIDJSON_USE_MEMBERSMAP
                        Map*& map = GetMap(GetMembersPointer());
                        MapIterator* mit = GetMapIterators(map);
                    #endif
                    for(MemberIterator itr=pos;itr!=last;++itr){// 从first删到last
                        #if RAPIDJSON_USE_MEMBERSMAP
                            map->erase(DropMapIterator(mit[itr-beg]));// 从映射表中删除first到last的成员
                        #endif
                        itr->~Member();
                    }
                    #if RAPIDJSON_USE_MEMBERSMAP
                        if(first!=last){// 将last后面的成员移到first这个已删除的起点
                            MemberIterator next = pos+(last-first);// last的后一个未删除成员
                            for(MemberIterator itr=pos;next!=end;++itr,++next) {// 将last后未删除的成员移到前面来
                                std::memcpy(static_cast<void*>(&*itr), &*next, sizeof(Member));
                                SizeType mpos = static_cast<SizeType>(itr-beg);
                                new (&mit[mpos]) MapIterator(DropMapIterator(mit[next-beg]));
                                mit[mpos]->second = mpos;
                            }
                        }   
                    #else
                        std::memmove(static_cast<void*>(&*pos), &*last, static_cast<size_t>(end-last)*sizeof(Member));// 直接将last后面的成员移到前面的空位,即覆盖已删除的成员
                    #endif
                    o.size -= static_cast<SizeType>(last-first);
                    return pos;
                }
                /**
                 * @brief 将指定的JSON对象rhs的成员复制到当前对象中
                 * 
                 * @tparam SourceAllocator 
                 * @param rhs 
                 * @param allocator 
                 * @param copyConstStrings 
                 */
                template<typename SourceAllocator>
                void DoCopyMembers(const GenericValue<Encoding, SourceAllocator>& rhs, Allocator& allocator, bool copyConstStrings) {
                    RAPIDJSON_ASSERT(rhs.GetType==kObjectType);
                    data_.f.flags = kObjectFlag;
                    SizeType count = rhs.data_.o.size;
                    Member* lm = DoAllocMembers(count, allocator);// 分配rhs这个JSON对象这么多的成员的内存
                    const typename GenericValue<Encoding, SourceAllocator>::Member* rm = rhs.GetMembersPointer();
                    #if RAPIDJSON_USE_MEMBERSMAP
                        Map*& map = GetMap(lm);
                        MapIterator* mit = GetMapIterator(map);
                    #endif
                    for(SizeType i=0;i<count;i++){// 逐个成员地复制rhs的成员名称和成员值
                        new (&lm[i].name) GenericValue(rm[i].name, allocator, copyConstStrings);
                        new (&lm[i].value) GenericValue(rm[i].value, allocator, copyConstStrings);
                    #if RAPIDJSON_USE_MEMBERSMAP// 启用了map,则在map中插入新的键值对,构建相应迭代器并复制到mit
                        new (&mit[i]) MapIterator(map->insert(MapPair(lm[i].name.data_.i)));
                    #endif
                    }
                    data_.o.size = data_.o.capacity = count;// 将当前JSON对象的size就设成rhs的成员数
                    SetMembersPointer(lm);// 将临时Member[]数组lm拷贝到当前对象的members中,即data_.o.members
                }
                /**
                 * @brief 设置当前GenericValue对象为一个JSON数组
                 * 
                 * @param values 
                 * @param count 
                 * @param allocator 
                 */
                void SetArrayRaw(GenericValue* values, SizeType count, Allocator& allocator) {
                    data_.f.flags = kArrayFlag;
                    if(count) {// 确保数组不为空
                        GenericValue* e = static_cast<GenericValue*>(allocator.Malloc(count*sizeof(GenericValue)));
                        SetElementsPointer(e);// 将当前JSON数组地址设置为e,即data_.a.elements=e
                        std::memcpy(static_cast<void*>(e), values, count*sizeof(GenericValue));// 将count个GenericValue值拷贝到data_.a.elements地址处
                    }
                    else
                        SetElementsPointer(0);
                    data_.a.size = data_.a.capacity = count;
                }
                /**
                 * @brief 设置当前GenericValue对象为一个JSON对象
                 * 
                 * @param members 
                 * @param count 
                 * @param allocator 
                 */
                void SetObjectRaw(Member* members, SizeType count, Allocator& allocator) {
                    data_.f.flags = kObjectFlag;
                    if(count) {// 确保成员不为空
                        Member* m = DoAllocMembers(count, allocator);
                        SetMembersPointer(m);// 将当前JSON对象地址设置为m,即data_.a.members=m
                        std::memcpy(static_cast<void*>(m), members, count*sizeof(Member));// 将count个GenericValue值拷贝到data_.a.members地址处
                        #if RAPIDJSON_USE_MEMBERSMAP
                            Map*& map = GetMap(m);
                            MapIterator* mit = GetMapIterators(map);
                            for(SizeType i=0;i<count;i++) 
                                new (&mit[i]) MapIterator(map->insert(MapPair(m[i].name.data_, i)));// 将count个成员复制到映射表中
                        #endif
                    }
                    else
                        SetMembersPointer(0);
                    data_.o.size = data_.o.capacity = count;
                }
                /**
                 * @brief 用于设置当前GenericValue对象为一个JSON字符串
                 * 
                 * @param s 
                 */
                void SetStringRaw(StringRefType s) RAPIDJSON_NOEXCEPT {
                    data_.f.flags = kConstStringFlag;// 设置为常量字符串
                    SetStringPointer(s);// 将当前JSON字符串地址设置为s,即data_.a.s=s
                    data_.s.length = s.length;
                }
                /**
                 * @brief 用于设置当前GenericValue对象为一个JSON字符串
                 * 根据字符串长度决定使用短字符串存储还是动态分配内存来存储长字符串
                 * @param s 
                 * @param allocator 
                 */
                void SetStringRaw(StringRefType s, Allocator& allocator) {
                    Ch* str = 0;
                    if(ShortString::Usable(s.length)) {// 短字符串 ShortString
                        data_.f.flags = kShortStringFlag;
                        data_.ss.SetLength(s.length);
                        str = data_.ss.str;
                    }
                    else {// 长字符串 String
                        data_.f.flags = kCopyStringFlag;
                        data_.s.length = s.length;
                        str = static_cast<Ch*>(allocator.Malloc((s.length+1)*sizeof(Ch));
                        SetStringPointer(str);
                    }
                    std::memcpy(str, s, s.length*sizeof(Ch));// 将s复制到str,因为data_.ss.str或data_.s.str与str指向同一个地址,所以直接复制到str即可
                    str[s.length] = '\0';
                }
                /**
                 * @brief 用于将一个指定的GenericValue对象的所有数据直接赋值给当前对象
                 * 
                 * @param rhs 
                 */
                void RawAssign(GenericValue& rhs) RAPIDJSON_NOEXCEPT {
                    data_ = rhs.data_;
                    rhs.data_.f.flags = kNullFlag;
                }
                /**
                 * @brief 用于比较当前GenericValue对象与另一个GenericValue对象rhs的字符串是否相等
                 * 
                 * @tparam SourceAllocator 
                 * @param rhs 
                 * @return true 
                 * @return false 
                 */
                template<typename SourceAllocator>
                bool StringEqual(const GenericValue<Encoding, SourceAllocator>& rhs) const {
                    RAPIDJSON_ASSERT(IsString());
                    RAPIDJSON_ASSERT(rhs.IsString());
                    const SizeType len1 = GetStringLength();
                    const SizeType len2 = rhs.GetStringLength();
                    if(len1!=len2)// 长度不等直接返回false
                        return false;
                    const Ch* const str1 = GetString();
                    const Ch* const str2 = rhs.GetString();
                    if(str1==str2)// 长度相等的前提下,若两个字符串指针相同,表示它们指向的是同一块内存,此时直接返回true
                        return true;
                    return (std::memcmp(str1, str2, sizeof(Ch)*len1)==0);// 长度相等的前提下,使用memcmp进行一个字节一个字节的比较
                }
                Data data_;// 当前GenericValue对象的值,即JSON数据的一种值(可能是对象值、数组值、字符串值等等)
    };
    




}

#endif