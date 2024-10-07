// Writer类用于将C++中的数据序列化为JSON格式
#ifndef RAPIDJSON_WRITER_H_
#define RAPIDJSON_WRITER_H_

#include "stream.h"
#include "internal/clzll.h"
#include "internal/meta.h"
#include "internal/stack.h"
#include "internal/strfunc.h"
#include "internal/dtoa.h"
#include "internal/itoa.h"
#include "stringbuffer.h"
#include <new>

namesapce RAPIDJSON{
#ifndef RAPIDJSON_WRITE_DEFAULT_FLAGS
#define RAPIDJSON_WRITE_DEFAULT_FLAGS kWriteNoFlags
#endif
// 枚举Writer类的写入标志,用于控制序列化行为
enum WriteFlag {
    kWriteNoFlags = 0,              // 没有设置任何标志
    kWriteValidateEncodingFlag = 1, // 验证JSON字符串的编码
    kWriteNanAndInfFlag = 2,        // 允许写入Infinity、-Infinity和Nan
    kWriteNanAndInfNullFlag = 4,    // 将Infinity、-Infinity和Nan写作null
    kWriteDefaultFlags = RAPIDJSON_WRITE_DEFAULT_FLAGS  // 默认写入标志,可通过定义 RAPIDJSON_WRITE_DEFAULT_FLAGS 自定义
};
/**
 * @brief 
 * 
 * @tparam OutputStream 
 * @tparam SourceEncoding 源编码,仅限于UTF8
 * @tparam TargetEncoding 目标编码,仅限于UTF8
 * @tparam StackAllocator 栈分配器,用于内存管理
 * @tparam writeFlags 写入标志,控制序列化行为,默认为kWriteDefaultFlags
 */
template<typename OutputStream, typename SourceEncoding = UTF8<>, typename TargetEncoding = UTF8<>, typename StackAllocator = CrtAllocator, unsigned writeFlags = kWriteDefaultFlags>
class Writer {
public:
    typedef typename SourceEncoding::Ch Ch;// 定义字符类型ch,根据源编码确定

    static const int kDefaultMaxDecimalPlaces = 324;// 默认的最大小位数,用于序列化浮点数

    //! Constructor
    /*! \param os Output stream.
        \param stackAllocator User supplied allocator. If it is null, it will create a private one.
        \param levelDepth Initial capacity of stack.
    */
    explicit
    Writer(OutputStream& os, StackAllocator* stackAllocator = 0, size_t levelDepth = kDefaultLevelDepth) : 
        os_(&os), level_stack_(stackAllocator, levelDepth * sizeof(Level)), maxDecimalPlaces_(kDefaultMaxDecimalPlaces), hasRoot_(false) {}

    explicit
    Writer(StackAllocator* allocator = 0, size_t levelDepth = kDefaultLevelDepth) :
        os_(0), level_stack_(allocator, levelDepth * sizeof(Level)), maxDecimalPlaces_(kDefaultMaxDecimalPlaces), hasRoot_(false) {}


    /**
     * @brief 重置Writer,以便复用同一个对象输出多个JSON
     * 
     * @param os 
     */
    void Reset(OutputStream& os) {
        os_ = &os;// 更新输出流
        hasRoot_ = false;// 重置根元素标记
        level_stack_.Clear();// 清空栈
    }

    /**
     * @brief 检查JSON是否完整,即是否有且仅有一个根元素,且对所有对象和数组都已正确闭合
     * 
     * @return true 
     * @return false 
     */
    bool IsComplete() const {
        return hasRoot_ && level_stack_.Empty();
    }
    /**
     * @brief Get the Max Decimal Places object
     * 
     * @return int 
     */
    int GetMaxDecimalPlaces() const {
        return maxDecimalPlaces_;
    }
    /**
     * @brief Set the Max Decimal Places object
     * 
     * @param maxDecimalPlaces 
     */
    void SetMaxDecimalPlaces(int maxDecimalPlaces) {
        maxDecimalPlaces_ = maxDecimalPlaces;
    }
    /**
     * @brief 实现RapidJSON的Handler借口,用于处理各种类型的数据
     * 
     * @return true 
     * @return false 
     */
    bool Null()                 { Prefix(kNullType);   return EndValue(WriteNull()); }// 写入一个JSON null值
    bool Bool(bool b)           { Prefix(b ? kTrueType : kFalseType); return EndValue(WriteBool(b)); }// 写入一个布尔值true或false
    // 写入各种类型的整数
    bool Int(int i)             { Prefix(kNumberType); return EndValue(WriteInt(i)); }
    bool Uint(unsigned u)       { Prefix(kNumberType); return EndValue(WriteUint(u)); }
    bool Int64(int64_t i64)     { Prefix(kNumberType); return EndValue(WriteInt64(i64)); }
    bool Uint64(uint64_t u64)   { Prefix(kNumberType); return EndValue(WriteUint64(u64)); }
    bool Double(double d)       { Prefix(kNumberType); return EndValue(WriteDouble(d)); }// 写入浮点数
    /**
     * @brief 直接写入未经解析的数字字符串,用于控制数字格式
     * 
     * @param str 
     * @param length 
     * @param copy 
     * @return true 
     * @return false 
     */
    bool RawNumber(const Ch* str, SizeType length, bool copy = false) {
        RAPIDJSON_ASSERT(str != 0);
        (void)copy;
        Prefix(kNumberType);
        return EndValue(WriteString(str, length));
    }
    /**
     * @brief 写入字符串
     * 
     * @param str 
     * @param length 
     * @param copy 
     * @return true 
     * @return false 
     */
    bool String(const Ch* str, SizeType length, bool copy = false) {
        RAPIDJSON_ASSERT(str != 0);
        (void)copy;
        Prefix(kStringType);
        return EndValue(WriteString(str, length));
    }


    bool StartObject() {
        Prefix(kObjectType);
        new (level_stack_.template Push<Level>()) Level(false);
        return WriteStartObject();
    }

    bool Key(const Ch* str, SizeType length, bool copy = false) { return String(str, length, copy); }


    bool EndObject(SizeType memberCount = 0) {
        (void)memberCount;
        RAPIDJSON_ASSERT(level_stack_.GetSize() >= sizeof(Level)); // not inside an Object
        RAPIDJSON_ASSERT(!level_stack_.template Top<Level>()->inArray); // currently inside an Array, not Object
        RAPIDJSON_ASSERT(0 == level_stack_.template Top<Level>()->valueCount % 2); // Object has a Key without a Value
        level_stack_.template Pop<Level>(1);
        return EndValue(WriteEndObject());
    }

    bool StartArray() {
        Prefix(kArrayType);
        new (level_stack_.template Push<Level>()) Level(true);
        return WriteStartArray();
    }

    bool EndArray(SizeType elementCount = 0) {
        (void)elementCount;
        RAPIDJSON_ASSERT(level_stack_.GetSize() >= sizeof(Level));
        RAPIDJSON_ASSERT(level_stack_.template Top<Level>()->inArray);
        level_stack_.template Pop<Level>(1);
        return EndValue(WriteEndArray());
    }
    //@}

    /*! @name Convenience extensions */
    //@{

    //! Simpler but slower overload.
    bool String(const Ch* const& str) { return String(str, internal::StrLen(str)); }
    bool Key(const Ch* const& str) { return Key(str, internal::StrLen(str)); }
    
    //@}

    //! Write a raw JSON value.
    /*!
        For user to write a stringified JSON as a value.

        \param json A well-formed JSON value. It should not contain null character within [0, length - 1] range.
        \param length Length of the json.
        \param type Type of the root of json.
    */
    bool RawValue(const Ch* json, size_t length, Type type) {
        RAPIDJSON_ASSERT(json != 0);
        Prefix(type);
        return EndValue(WriteRawValue(json, length));
    }

    //! Flush the output stream.
    /*!
        Allows the user to flush the output stream immediately.
     */
    void Flush() {
        os_->Flush();
    }

    static const size_t kDefaultLevelDepth = 32;

protected:
    //! Information for each nested level
    struct Level {
        Level(bool inArray_) : valueCount(0), inArray(inArray_) {}
        size_t valueCount;  //!< number of values in this level
        bool inArray;       //!< true if in array, otherwise in object
    };

    bool WriteNull()  {
        PutReserve(*os_, 4);
        PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 'l'); return true;
    }

    bool WriteBool(bool b)  {
        if (b) {
            PutReserve(*os_, 4);
            PutUnsafe(*os_, 't'); PutUnsafe(*os_, 'r'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'e');
        }
        else {
            PutReserve(*os_, 5);
            PutUnsafe(*os_, 'f'); PutUnsafe(*os_, 'a'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 's'); PutUnsafe(*os_, 'e');
        }
        return true;
    }

    bool WriteInt(int i) {
        char buffer[11];
        const char* end = internal::i32toa(i, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (const char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
        return true;
    }

    bool WriteUint(unsigned u) {
        char buffer[10];
        const char* end = internal::u32toa(u, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (const char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
        return true;
    }

    bool WriteInt64(int64_t i64) {
        char buffer[21];
        const char* end = internal::i64toa(i64, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (const char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
        return true;
    }

    bool WriteUint64(uint64_t u64) {
        char buffer[20];
        char* end = internal::u64toa(u64, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
        return true;
    }

    bool WriteDouble(double d) {
        if (internal::Double(d).IsNanOrInf()) {
            if (!(writeFlags & kWriteNanAndInfFlag) && !(writeFlags & kWriteNanAndInfNullFlag))
                return false;
            if (writeFlags & kWriteNanAndInfNullFlag) {
                PutReserve(*os_, 4);
                PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 'l');
                return true;
            }
            if (internal::Double(d).IsNan()) {
                PutReserve(*os_, 3);
                PutUnsafe(*os_, 'N'); PutUnsafe(*os_, 'a'); PutUnsafe(*os_, 'N');
                return true;
            }
            if (internal::Double(d).Sign()) {
                PutReserve(*os_, 9);
                PutUnsafe(*os_, '-');
            }
            else
                PutReserve(*os_, 8);
            PutUnsafe(*os_, 'I'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'f');
            PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 't'); PutUnsafe(*os_, 'y');
            return true;
        }

        char buffer[25];
        char* end = internal::dtoa(d, buffer, maxDecimalPlaces_);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
        return true;
    }

    bool WriteString(const Ch* str, SizeType length)  {
        static const typename OutputStream::Ch hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static const char escape[256] = {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
            'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
            'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
              0,   0, '"',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20
            Z16, Z16,                                                                       // 30~4F
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,'\\',   0,   0,   0, // 50
            Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16                                // 60~FF
#undef Z16
        };

        if (TargetEncoding::supportUnicode)
            PutReserve(*os_, 2 + length * 6); // "\uxxxx..."
        else
            PutReserve(*os_, 2 + length * 12);  // "\uxxxx\uyyyy..."

        PutUnsafe(*os_, '\"');
        GenericStringStream<SourceEncoding> is(str);
        while (ScanWriteUnescapedString(is, length)) {
            const Ch c = is.Peek();
            if (!TargetEncoding::supportUnicode && static_cast<unsigned>(c) >= 0x80) {
                // Unicode escaping
                unsigned codepoint;
                if (RAPIDJSON_UNLIKELY(!SourceEncoding::Decode(is, &codepoint)))
                    return false;
                PutUnsafe(*os_, '\\');
                PutUnsafe(*os_, 'u');
                if (codepoint <= 0xD7FF || (codepoint >= 0xE000 && codepoint <= 0xFFFF)) {
                    PutUnsafe(*os_, hexDigits[(codepoint >> 12) & 15]);
                    PutUnsafe(*os_, hexDigits[(codepoint >>  8) & 15]);
                    PutUnsafe(*os_, hexDigits[(codepoint >>  4) & 15]);
                    PutUnsafe(*os_, hexDigits[(codepoint      ) & 15]);
                }
                else {
                    RAPIDJSON_ASSERT(codepoint >= 0x010000 && codepoint <= 0x10FFFF);
                    // Surrogate pair
                    unsigned s = codepoint - 0x010000;
                    unsigned lead = (s >> 10) + 0xD800;
                    unsigned trail = (s & 0x3FF) + 0xDC00;
                    PutUnsafe(*os_, hexDigits[(lead >> 12) & 15]);
                    PutUnsafe(*os_, hexDigits[(lead >>  8) & 15]);
                    PutUnsafe(*os_, hexDigits[(lead >>  4) & 15]);
                    PutUnsafe(*os_, hexDigits[(lead      ) & 15]);
                    PutUnsafe(*os_, '\\');
                    PutUnsafe(*os_, 'u');
                    PutUnsafe(*os_, hexDigits[(trail >> 12) & 15]);
                    PutUnsafe(*os_, hexDigits[(trail >>  8) & 15]);
                    PutUnsafe(*os_, hexDigits[(trail >>  4) & 15]);
                    PutUnsafe(*os_, hexDigits[(trail      ) & 15]);                    
                }
            }
            else if ((sizeof(Ch) == 1 || static_cast<unsigned>(c) < 256) && RAPIDJSON_UNLIKELY(escape[static_cast<unsigned char>(c)]))  {
                is.Take();
                PutUnsafe(*os_, '\\');
                PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(escape[static_cast<unsigned char>(c)]));
                if (escape[static_cast<unsigned char>(c)] == 'u') {
                    PutUnsafe(*os_, '0');
                    PutUnsafe(*os_, '0');
                    PutUnsafe(*os_, hexDigits[static_cast<unsigned char>(c) >> 4]);
                    PutUnsafe(*os_, hexDigits[static_cast<unsigned char>(c) & 0xF]);
                }
            }
            else if (RAPIDJSON_UNLIKELY(!(writeFlags & kWriteValidateEncodingFlag ? 
                Transcoder<SourceEncoding, TargetEncoding>::Validate(is, *os_) :
                Transcoder<SourceEncoding, TargetEncoding>::TranscodeUnsafe(is, *os_))))
                return false;
        }
        PutUnsafe(*os_, '\"');
        return true;
    }

    bool ScanWriteUnescapedString(GenericStringStream<SourceEncoding>& is, size_t length) {
        return RAPIDJSON_LIKELY(is.Tell() < length);
    }

    bool WriteStartObject() { os_->Put('{'); return true; }
    bool WriteEndObject()   { os_->Put('}'); return true; }
    bool WriteStartArray()  { os_->Put('['); return true; }
    bool WriteEndArray()    { os_->Put(']'); return true; }

    bool WriteRawValue(const Ch* json, size_t length) {
        PutReserve(*os_, length);
        GenericStringStream<SourceEncoding> is(json);
        while (RAPIDJSON_LIKELY(is.Tell() < length)) {
            RAPIDJSON_ASSERT(is.Peek() != '\0');
            if (RAPIDJSON_UNLIKELY(!(writeFlags & kWriteValidateEncodingFlag ? 
                Transcoder<SourceEncoding, TargetEncoding>::Validate(is, *os_) :
                Transcoder<SourceEncoding, TargetEncoding>::TranscodeUnsafe(is, *os_))))
                return false;
        }
        return true;
    }

    void Prefix(Type type) {
        (void)type;
        if (RAPIDJSON_LIKELY(level_stack_.GetSize() != 0)) { // this value is not at root
            Level* level = level_stack_.template Top<Level>();
            if (level->valueCount > 0) {
                if (level->inArray) 
                    os_->Put(','); // add comma if it is not the first element in array
                else  // in object
                    os_->Put((level->valueCount % 2 == 0) ? ',' : ':');
            }
            if (!level->inArray && level->valueCount % 2 == 0)
                RAPIDJSON_ASSERT(type == kStringType);  // if it's in object, then even number should be a name
            level->valueCount++;
        }
        else {
            RAPIDJSON_ASSERT(!hasRoot_);    // Should only has one and only one root.
            hasRoot_ = true;
        }
    }

    // Flush the value if it is the top level one.
    bool EndValue(bool ret) {
        if (RAPIDJSON_UNLIKELY(level_stack_.Empty()))   // end of json text
            Flush();
        return ret;
    }

    OutputStream* os_;
    internal::Stack<StackAllocator> level_stack_;
    int maxDecimalPlaces_;
    bool hasRoot_;

private:
    // Prohibit copy constructor & assignment operator.
    Writer(const Writer&);
    Writer& operator=(const Writer&);
};

// Full specialization for StringStream to prevent memory copying

template<>
inline bool Writer<StringBuffer>::WriteInt(int i) {
    char *buffer = os_->Push(11);
    const char* end = internal::i32toa(i, buffer);
    os_->Pop(static_cast<size_t>(11 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteUint(unsigned u) {
    char *buffer = os_->Push(10);
    const char* end = internal::u32toa(u, buffer);
    os_->Pop(static_cast<size_t>(10 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteInt64(int64_t i64) {
    char *buffer = os_->Push(21);
    const char* end = internal::i64toa(i64, buffer);
    os_->Pop(static_cast<size_t>(21 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteUint64(uint64_t u) {
    char *buffer = os_->Push(20);
    const char* end = internal::u64toa(u, buffer);
    os_->Pop(static_cast<size_t>(20 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteDouble(double d) {
    if (internal::Double(d).IsNanOrInf()) {
        // Note: This code path can only be reached if (RAPIDJSON_WRITE_DEFAULT_FLAGS & kWriteNanAndInfFlag).
        if (!(kWriteDefaultFlags & kWriteNanAndInfFlag))
            return false;
        if (kWriteDefaultFlags & kWriteNanAndInfNullFlag) {
            PutReserve(*os_, 4);
            PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 'l');
            return true;
        }
        if (internal::Double(d).IsNan()) {
            PutReserve(*os_, 3);
            PutUnsafe(*os_, 'N'); PutUnsafe(*os_, 'a'); PutUnsafe(*os_, 'N');
            return true;
        }
        if (internal::Double(d).Sign()) {
            PutReserve(*os_, 9);
            PutUnsafe(*os_, '-');
        }
        else
            PutReserve(*os_, 8);
        PutUnsafe(*os_, 'I'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'f');
        PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 't'); PutUnsafe(*os_, 'y');
        return true;
    }
    
    char *buffer = os_->Push(25);
    char* end = internal::dtoa(d, buffer, maxDecimalPlaces_);
    os_->Pop(static_cast<size_t>(25 - (end - buffer)));
    return true;
}
};
#endif