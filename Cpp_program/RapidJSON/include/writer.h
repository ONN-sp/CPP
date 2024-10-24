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
#include <new>// placement new

// SSE指令集用于提供一系列操作 用于加速向量和矩阵计算
#ifdef RAPIDJSON_SSE42
#include <nmmintrin.h>
#elif defined(RAPIDJSON_SSE2)
#include <emmintrin.h>
#elif defined(RAPIDJSON_NEON)
#include <arm_neon.h>
#endif

namespace RAPIDJSON{
    #ifndef RAPIDJSON_WRITE_DEFAULT_FLAGS
    #define RAPIDJSON_WRITE_DEFAULT_FLAGS kWriteNoFlags// 默认情况下,没有标志
    #endif
    enum WriteFlag{
        kWriteNoFlags = 0,// 无标志
        kWriteValidateEncodingFlag = 1,// 验证JSON字符串的编码 本项目只有UTF8
        kWriteNanAndInfFlag = 2,// 允许写入Infinity, -Infinity和Nan
        kWriteNanAndInfNullFlag = 4,// 允许将Infinity, -Infinity和Nan写为Null
        kWriteDefaultFlags = RAPIDJSON_WRITE_DEFAULT_FLAGS// 默认写入标志
    };
    template<typename OutputStream, typename SourceEncoding = UTF8<>, typename TargetEncoding = UTF8<>, typename StackAllocator = CrtAllocator, unsigned writeFlags = kWriteDefaultFlags>
    class Writer{
        public:
            typedef typename SourceEncoding::Ch Ch;
            static const int kDefaultMaxDecimalPlaces = 324;// 默认的最大十进制位数  在将双精度数转换为JSON字符串时,这个常量限制了小数部分的位数
            /**
             * @brief Construct a new Writer object
             * 
             * @param os 
             * @param StackAllocator 
             * @param levelDepth 栈的初始容量
             */
            explicit Writer(OutputStream& os, StackAllocator* StackAllocator = nullptr, size_t levelDepth = kDefaultLevelDepth)
                : os_(&os),
                  level_stack_(allocator, levelDepth*sizeof(Level)),
                  maxDecimalPlaces_(kDefaultMaxDecimalPlaces),
                  hasRoot_(false)
                {}
            explicit Writer(StackAllocator* StackAllocator = nullptr, size_t levelDepth = kDefaultLevelDepth)
                : os_(nullptr),
                  level_stack_(allocator, levelDepth*sizeof(Level)),
                  maxDecimalPlaces_(kDefaultMaxDecimalPlaces),
                  hasRoot_(false)
                {}
            #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
            Writer(Writer&& rhs)
                : os_(rhs.os_),
                  level_stack_(std::move(rhs.level_stack_)),// Stack.h中有移动构造函数
                  maxDecimalPlaces_(rhs.kDefaultMaxDecimalPlaces),
                  hasRoot_(rhs.hasRoot_)
                {
                    rhs.os_ = nullptr;
                }
            #endif
            void Reset(OutputStream& os){
                os_ = &os;// 更新输出流
                hasRoot_ = false;// 重置根标志
                level_stack_.Clear();// 清空栈
            }
            bool IsComplete() const {
                return hasRoot_ && level_stack_.Empty();// 检测是否有根并且栈是否为空
            }
            int GetMaxDecimalPlaces() const {
                return maxDecimalPlaces_;// 返回最大十进制位数
            }
            void SetMaxDecimalPlaces(int maxDecimalPlaces){
                maxDecimalPlaces_ = maxDecimalPlaces;// 更新最大十进制位数
            }
            // 定义JSON中的各种类型
            bool Null() {  
                Prefix(kNullType);
                return EndValue(WriteNull());
            }
            bool Bool(bool b){
                Prefix(b ? kTrueType : kFalseType);
                return  EndValue(WriteBool(b));
            }
            bool Int(int i){
                Prefix(kNumberType);
                return EndValue(WriteInt(i));
            }
            bool Uint(unsigned u){
                Prefix(kNumberType);
                return EndValue(WriteUint(u));
            }
            bool Int64(int64_t i64){
                Prefix(kNumberType);
                return EndValue(WriteInt64(i64));
            }
            bool Uint64(uint64_t u64){
                Prefix(kNumberType);
                return EndValue(WriteUint64(u64));
            }
            bool Double(double d){
                Prefix(kNumberType);
                return EndValue(WriteDouble(d));
            }
            bool RawNumber(const Ch* str, SizeType length, bool copy=false){
                RAPIDJSON_ASSERT(str!=0);
                (void)copy;
                Prefix(kNumberType);// 设置前缀
                return EndValue(WriteString(str, length));// 写入原始数字
            }
            bool String(const Ch* str, SizeType length, bool copy = false){
                RAPIDJSON_ASSERT(str!=0);
                (void)copy;
                Prefix(kStringType);
                return EndValue(WriteString(str, length));
            }
            #if RAPIDJSON_HAS_STDSTRING
            bool String(const std::basic_string<Ch>& str){
                return String(str.data(), SizeType(str.size()));// 支持std::string的重载
            }
            #endif
            bool StartObject(){
                Prefix(kObjectType);
                new (level_stack_.template Push<Level>()) Level(false);
                return WriteStartObject();// 写入开始对象
            }
            bool Key(const Ch* str, SizeType length, bool copy = false){
                return String(str, length, copy);// 写入键
            }
            #if RAPIDJSON_HAS_STDSTRING
            bool Key(const std::basic_string<Ch>& str){
                return Key(str.data(), SizeType(str.size()));
            }
            #endif
            bool EndObject(SizeType memberCount = 0){
                (void)memberCount;
                RAPIDJSON_ASSERT(level_stack_.GetSize() >= sizeof(Level));
                RAPIDJSON_ASSERT(!level_stack_.template Top<Level>()->inArray);
                RAPIDJSON_ASSERT(0==level_stack_.template Top<Level>()->valueCount);
                level_stack_.template Pop<Level>(1);// 因为当前对象结束了,所以弹出当前级的嵌套层次
                return EndValue(WriteEndObject());// 写入结束对象
            }
            bool StartArray(){
                Prefix(kArrayType);
                new (level_stack_.template Push<Level>())  Level(true);
                return WriteStartArray();
            }
            bool EndArray(SizeType elementCount=0){
                (void)elementCount;
                RAPIDJSON_ASSERT(level_stack_.GetSize() >= sizeof(Level));
                RAPIDJSON_ASSERT(level_stack_.template Top<Level>()->inArray);
                level_stack_.template Pop<Level>(1);
                return EndValue(WriteEndArray());
            }
            bool String(const Ch* constr& str) {return String(str, internal::StrLen(str));}// 写入JSON的字符串值
            bool Key(const Ch* const& str) {return Key(str, internal::Strlen(str));}// 写入JSON的键值对的键
            /**
             * @brief 将已经序列化的JSON字符串(已经按照JSON格式编码过的数据)作为原始值直接写入输出流
             * 
             * @param json 
             * @param length 
             * @param type 
             * @return true 
             * @return false 
             */
            bool RawValue(const Ch* json, size_t length, Type type){
                RAPIDJSON_ASSERT(json!=nullptr);// 确保传入的字符串不为空
                Prefix(type);// 添加必要的前缀(如逗号或冒号),确保JSON结构正确
                return EndValue(WriteRawValue(json, length));// 写入原始JSON字符串
            }
            void Flush(){
                os_->Flush();
            }
            static const size_T kDefaultLevelDepth = 32;// 默认生成的JSON数据的嵌套层次深度,即默认可以处理嵌套深度为32层的对象或数组
        protected:
            /**
             * @brief 
             * 作用:
             * 1. 用于在写JSON时维护嵌套层次,跟踪当前的层次状态
             * 2. 区分对象和数组,确保正确写入键值对或数组元素
             * 3. 对于对象,跟踪对象中的键值对数量,确保每个键都有值
             * 4. 在写入嵌套结构时,确保最终生成的JSON是合法且完整的
             */
            struct Level {
                Level(bool inArray_) : valueCount(0), inArray(inArray_) {}
                size_t valueCount;// 当前对象或数组中的值计数器,用来记录已写入的键值对或数组元素的数量  对象(键值对)  数组(数组元素)
                bool inArray;// 表示当前这个层级是对象还是数组(true表示数组,false表示对象)
            };
            // 写入一个NULL值到JSON输出中
            bool WriteNull(){
                PutReserve(*os_, 4);// 预留4个字符的空间("null")
                PutUnsafe(*os_, 'n');
                PutUnsafe(*os_, 'u');
                PutUnsafe(*os_, 'l');
                PutUnsafe(*os_, 'l');
                return true;
            }
            // 根据bool值是true还是false  写入对应的true和false字符串到JSON输出中
            bool WriteBool(bool b){
                if(b){
                    PutReserve(*os_, 4);
                    PutUnsafe(*os_, 't');
                    PutUnsafe(*os_, 'u');
                    PutUnsafe(*os_, 'r');
                    PutUnsafe(*os_, 'e');
                }
                else{
                    PutReserve(*os_, 5);
                    PutUnsafe(*os_, 'f');
                    PutUnsafe(*os_, 'a');
                    PutUnsafe(*os_, 'l');
                    PutUnsafe(*os_, 's');
                    PutUnsafe(*os_, 'e');
                }
                return true;
            }
        bool WriteInt(int i){
            char buffer[11];// int最多10位数字,加上负号共11
            const char* end = internal::i32toa(i, buffer);// 将整数i转换为字符串
            PutReserve(*os_, static_cast<size_t>(end-buffer));
            for(const char*p=buffer;p!=end;++p)
                PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));// 写入每个字符
            return true;
        }
        bool WriteUint(unsigned u){
            char buffer[10];// 无符号
            const char* end = internal::u32toa(u, buffer);
            PutReserve(*os_, static_cast<size_t>(end-buffer));
            for(const char* p=buffer;p!=end;++p)
                PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
            return true;
        }
        bool WriteInt64(int64_t i64){
            char buffer[21];// int64最大20位数字+1位符号
            const char* end = internal::i64toa(i64, buffer);
            PutReserve(*os_, static_cast<size_t>(end-buffer));
            for(const char* p =buffer;p!=end;++p)
                PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
            return true;
        }
        bool WriteUint64(uint64_t u64){
            char buffer[20];// 无符号
            const char* end = internal::u64toa(u64, buffer);
            PutReserve(*os_, static_cast<size_t>(end-buffer));
            for(const char* p =buffer;p!=end;++p)
                PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
            return true;
        }
        bool WriteDouble(double d){
            if(internal::Double(d).InNanOrInf()){// 处理Nan或Inf的情况
                if(!(writeFlags&kWriteNanAndInfFlag)&&!(writeFlags&kWriteNanAndInfNullFlag))
                    return false;
                if(writeFlags&kWriteNanAndInfNullFlag){// 启用了kWriteNanAndInfNullFlag,则将Nan处理位JSON的null,并向输出流写入null
                    PutReserve(*os_, 4);
                    PutUnsafe(*os_, 'n');
                    PutUnsafe(*os_, 'u');
                    PutUnsafe(*os_, 'l');
                    PutUnsafe(*os_, 'l');
                    return true;
                }
                if(internal::Double(d).IsNan()){// 未启用kWriteNanAndInfNullFlag,但启用了kWriteNanAndInfFlag  则直接写入Nan
                    PutReserve(*os_, 3);
                    PutUnsafe(*os_, 'N');
                    PutUnsafe(*os_, 'a');
                    PutUnsafe(*os_, 'N');
                    return true;
                }
                if(internal::Double(d).Sign()){// 如果是负数
                    PutReserve(*os_, 9);
                    PutUnsafe(*os_. '-');
                }
                else
                    PutReserve(*os_, 8);
                PutUnsafe(*os_, 'I');
                PutUnsafe(*os_, 'n');
                PutUnsafe(*os_, 'f');
                PutUnsafe(*os_, 'i');
                PutUnsafe(*os_, 'n');
                PutUnsafe(*os_, 'i');
                PutUnsafe(*os_, 't');
                PutUnsafe(*os_, 'y');
                return true;
            }
            // 处理普通的double值
            char buffer[25];
            char* end = internal::dtoa(d, buffer, maxDecimalPlaces_);
            PutReserve(*os_, static_cast<size_t>(end-buffer));
            for(char* p=buffer;p!=end;++p)
                PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(*p));
            return true;
        }
        /**
         * @brief 将输入的字符串str序列化为JSON格式的字符串,并处理字符串中的特殊字符(如换行符、引号、反斜杠等)
         * 
         * @param str 
         * @param length 
         * @return true 
         * @return false 
         */
        bool WriteString(const Ch* str, SizeType length){
            // 用于将Unicode字符转换为十六进制
            static const typename OutputStream::Ch hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            // 定义转义字符表 该表定义了256个ASCII码字符的转义映射 可以用来查找某个字符是否需要转义(对应表中非0)
            // 不可打印字符(如控制字符) 使用'u'进行Unicode转义
            // 特殊字符(如\")需要转义,直接赋值'\\'或'"'
            // 其余字符不需要转义,值为0
            static const char escape[265] = {
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
            if(TargetEncoding::supportUnicode)// 若支持Unicode,每个字符最多需要6个字节来表示"\uxxxx"
                PutReserve(*os_, 2+length*6);// "\uxxxx..."
            else// 若不支持Unicode,则可能需要使用UTF16的代理对,因此要预留的空间更多
                PutReserve(*os_, 2+length*12);// "\uxxxx\uyyyy...":用两个Unicode转义序列表示一个字符  代理对
            PutUnsafe(*os_, '\"');// JSON中以字符串以双引号包围,因此首先写入起始的双引号
            GenericStringStream<SourceEncoding> is(str);// 创建一个字符串读取流对象  用于从输入字符串str中读取字符
            while(ScanWriteUnescapedString(is, length){// 扫描并写入非转义字符串
                const Ch c = is.Peek();// 查看当前字符
                if(!TargetEncoding::supportUnicode&&static_cast<unsigned>(c) >= 0x80){// 不支持Unicode且为非ASCII字符  此时需要将该字符转换为Unicode转义序列(即\uxxxx格式)  如:"世"不是ASCII码,若不支持Unicode,则需要将它转换为Unicode转义码\u4E16写入输出流才行
                    unsigned codepoint;
                    if(RAPIDJSON_UNLIKELY(!SourceEncoding::Decode(is, &codepoint)))// 解码输入流中的字符 UTF8->Unicode代码点
                        return false;
                    PutUnsafe(*os_, '\\');// 编译器会将这两个反斜杠视为一个反斜杠字符,因为在字符串中表示一个实际的反斜杠字符,需要使用双反斜杠\\ 
                    PutUnsafe(*os_, 'u');
                    if (codepoint <= 0xD7FF || (codepoint >= 0xE000 && codepoint <= 0xFFFF)) {// codepoint处于0x0000到0xD7FF之间的码点属于Unicode基本多语言平面(BMP)  这些Unicode字符之间将其转换为\uxxxx的形式
                        PutUnsafe(*os_, hexDigits[(codepoint >> 12) & 15]);
                        PutUnsafe(*os_, hexDigits[(codepoint >>  8) & 15]);
                        PutUnsafe(*os_, hexDigits[(codepoint >>  4) & 15]);
                        PutUnsafe(*os_, hexDigits[(codepoint      ) & 15]);
                    }
                    else{// ??? (用UTF16处理超出BMP的方式来处理,这与使用UTF8不冲突,因为后面写入输出流是用的UTF8格式)处理代码点超出BMP的Unicode字符  超出BMP的字符需要使用代理对来表示
                        RAPIDJSON_ASSERT(codepoint >= 0x010000 && codepoint <= 0x10FFFF);
                        unsigned s = codepoint - 0x010000;
                        unsigned lead = (s >> 10) + 0xD800;
                        unsigned trail = (s & 0x3FF) + 0xDC00;
                        // 高代理部分转化为\uxxxx格式的Unicode转义序列,并写入输出流
                        PutUnsafe(*os_, hexDigits[(lead >> 12) & 15]);
                        PutUnsafe(*os_, hexDigits[(lead >>  8) & 15]);
                        PutUnsafe(*os_, hexDigits[(lead >>  4) & 15]);
                        PutUnsafe(*os_, hexDigits[(lead      ) & 15]);
                        // 高代理后,添加\u
                        PutUnsafe(*os_, '\\');
                        PutUnsafe(*os_, 'u');
                        // 低代理部分也转化为\uxxxx的Unicode转义序列,并写入输出流
                        PutUnsafe(*os_, hexDigits[(trail >> 12) & 15]);
                        PutUnsafe(*os_, hexDigits[(trail >>  8) & 15]);
                        PutUnsafe(*os_, hexDigits[(trail >>  4) & 15]);
                        PutUnsafe(*os_, hexDigits[(trail      ) & 15]);   
                    }
                }
                // 处理转义字符 static_cast<unsigned>(c) < 256保证是ASCII字符
                else if ((sizeof(Ch) == 1 || static_cast<unsigned>(c) < 256) && RAPIDJSON_UNLIKELY(escape[static_cast<unsigned char>(c)]))  {
                    is.Take();
                    PutUnsafe(*os_, '\\');// 在输出流中写入反斜杠 '\' 表示转义符号
                    PutUnsafe(*os_, static_cast<typename OutputStream::Ch>(escape[static_cast<unsigned char>(c)]));
                    // 如果该字符在 escape 数组中对应的值为 'u'，则表示这个字符需要用 \uXXXX 形式转义  如果字符需要转义为 Unicode 编码形式(即\uXXXX),escape[] 表中的值会是 'u'.这种情况通常是针对不可见的控制字符(如:换行符、制表符、删除符、退格符等)或其它非打印字符
                    if (escape[static_cast<unsigned char>(c)] == 'u') {
                        PutUnsafe(*os_, '0');
                        PutUnsafe(*os_, '0');
                        PutUnsafe(*os_, hexDigits[static_cast<unsigned char>(c) >> 4]);
                        PutUnsafe(*os_, hexDigits[static_cast<unsigned char>(c) & 0xF]);
                    }
                }
                // 处理常规字符的转码
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
    };      
}
#endif