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
                    if(RAPIDJOSN_LIKELY(this!=&rhs)){
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
                    return d >= -3.4028234e38 && d<=3.4028234e38;
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




    };
    




}

#endif