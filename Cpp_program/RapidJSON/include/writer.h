// Writer类用于将JSON数据序列化为JSON字符串
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
             * @brief 构建一个紧凑输出到输出流的JSON编写器
             * 
             * @param os 
             * @param StackAllocator 
             * @param levelDepth 栈的初始容量
             */
            explicit Writer(OutputStream& os, StackAllocator* stackAllocator = nullptr, size_t levelDepth = kDefaultLevelDepth)
                : os_(&os),
                  level_stack_(stackAllocator, levelDepth*sizeof(Level)),
                  maxDecimalPlaces_(kDefaultMaxDecimalPlaces),
                  hasRoot_(false)
                {}
            explicit Writer(StackAllocator* stackAllocator = nullptr, size_t levelDepth = kDefaultLevelDepth)
                : os_(nullptr),
                  level_stack_(stackAllocator, levelDepth*sizeof(Level)),
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
                return EndValue(WriteBool(b));
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
                new (level_stack_.template Push<Level>()) Level(false);// false表示使一个对象而非数组
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
                RAPIDJSON_ASSERT(0==level_stack_.template Top<Level>()->valueCount % 2);// 确保对象的键值对数量为偶数,即有一个key就有一个value对应
                level_stack_.template Pop<Level>(1);// 因为当前对象结束了,所以弹出当前级的嵌套层次
                return EndValue(WriteEndObject());// 写入结束对象
            }
            bool StartArray(){
                Prefix(kArrayType);
                new (level_stack_.template Push<Level>()) Level(true);
                return WriteStartArray();
            }
            bool EndArray(SizeType elementCount=0){
                (void)elementCount;
                RAPIDJSON_ASSERT(level_stack_.GetSize() >= sizeof(Level));// 确保当前栈的大小>=Level大小,即确保当前栈至少包含一个完整的Level(true)结构(对于数组来说)
                RAPIDJSON_ASSERT(level_stack_.template Top<Level>()->inArray);
                level_stack_.template Pop<Level>(1);
                return EndValue(WriteEndArray());
            }
            bool String(const Ch* const& str) {return String(str, internal::StrLen(str));}// 写入JSON的字符串值
            bool Key(const Ch* const& str) {return Key(str, internal::StrLen(str));}// 写入JSON的键值对的键
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
            static const size_t kDefaultLevelDepth = 32;// 默认生成的JSON字符串的嵌套层次深度,即默认可以处理嵌套深度为32层的对象或数组
        protected:
            /**
             * @brief 
             * 作用:
             * 1. 用于在写JSON时维护嵌套层次,跟踪当前的层次状态
             * 2. 区分对象和数组,确保正确写入键值对或数组元素
             * 3. 对于对象,跟踪对象中的键值对数量,确保每个键都有值
             * 4. 在写入嵌套结构时,确保最终生成的JSON字符串是合法且完整的
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
            if(internal::Double(d).IsNanOrInf()){// 处理Nan或Inf的情况
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
                    PutUnsafe(*os_, '-');
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
            // 特殊字符(如'\')需要转义,直接赋值'\\'
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
            while(ScanWriteUnescapedString(is, length)){// 扫描输入流中的字符串 检查输入流is的当前位置是否小于指定的长度length,这用于确定字符串是否还有未处理的字符,确保不会超出字符串的边界
                const Ch c = is.Peek();// 查看当前字符
                if(!TargetEncoding::supportUnicode&&static_cast<unsigned>(c) >= 0x80){// 目标编码不支持Unicode(如ASCII编码)且为非ASCII字符(如:目标编码为ISO-8859-1)  此时需要将该字符转换为Unicode转义序列(即\uxxxx格式)  如:"世"不是ASCII码,若不支持Unicode,则需要将它转换为Unicode转义码\u4E16写入输出流才行
                    unsigned codepoint;
                    if(RAPIDJSON_UNLIKELY(!SourceEncoding::Decode(is, &codepoint)))// 解码输入流中的字符,前面判断的是目标编码不支持Unicode,这里是解码输入流,不要弄混了 UTF8->Unicode代码点
                        return false;
                    PutUnsafe(*os_, '\\');// 编译器会将这两个反斜杠视为一个反斜杠字符,因为在字符串中表示一个实际的反斜杠字符,需要使用双反斜杠\\ 
                    PutUnsafe(*os_, 'u');
                    if (codepoint <= 0xD7FF || (codepoint >= 0xE000 && codepoint <= 0xFFFF)) {// codepoint处于0x0000到0xD7FF之间的码点属于Unicode基本多语言平面(BMP)  这些Unicode字符之间将其转换为\uxxxx的形式
                        PutUnsafe(*os_, hexDigits[(codepoint >> 12) & 15]);
                        PutUnsafe(*os_, hexDigits[(codepoint >>  8) & 15]);
                        PutUnsafe(*os_, hexDigits[(codepoint >>  4) & 15]);
                        PutUnsafe(*os_, hexDigits[(codepoint      ) & 15]);
                    }
                    else{// (用UTF16处理超出BMP的方式来处理,这与使用UTF8不冲突,因为后面写入输出流是用的UTF8格式)处理代码点超出BMP的Unicode字符  超出BMP的字符需要使用代理对来表示
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
        // 只是检查is.Tell()<length是否成立,不进行任何字符串内容检查和写入输出流的操作
        bool ScanWriteUnescapedString(GenericStringStream<SourceEncoding>& is, size_t length) {
            return RAPIDJSON_LIKELY(is.Tell() < length);
        }
        bool WriteStartObject() { os_->Put('{'); return true; }
        bool WriteEndObject()   { os_->Put('}'); return true; }
        bool WriteStartArray()  { os_->Put('['); return true; }
        bool WriteEndArray()    { os_->Put(']'); return true; }  
        /**
         * @brief 将给定的JSON字符串原样写入输出流.这在处理已经格式化好的JSON数据时非常有用
         * 避免对字符串的二次解析或转义处理
         * 
         * @param json 
         * @param length 
         * @return true 
         * @return false 
         */
        bool WriteRawValue(const Ch* json, size_t length){
            PutReserve(*os_, length);
            GenericStringStream<SourceEncoding> is(json);// 创建一个从json字符串中读取字符的流对象
            while(RAPIDJSON_LIKELY(is.Tell() < length)){// 查当前读取位置是否小于字符串的总长度
                RAPIDJSON_ASSERT(is.Peek()!='\0');// 确保当前字符不为 null
                // 如果writeFlags中包含kWriteValidateEncodingFlag,则调用Validate方法来验证字符的编码;否则直接用TranscodeUnsafe将字符写入输出流
                if(RAPIDJSON_UNLIKELY(!(writeFlags&kWriteValidateEncodingFlag?
                    Transcoder<SourceEncoding, TargetEncoding>::Validate(is, *os_) :
                    Transcoder<SourceEncoding, TargetEncoding>::TranscodeUnsafe(is, *os_))))
                    return false;
            }
            return true;
        }
        /**
         * @brief 在JSON序列化时处理元素的前缀,以确保格式正确
         * 通常在写入一个新的值之前调用,用于确定是否需要添加逗号、冒号等符号,还会检查根元素的合法性
         * 
         * @param type 
         */
        void Prefix(Type type){
            (void)type;
            if(RAPIDJSON_LIKELY(level_stack_.GetSize()!=0)){// 表示当前正在处理的元素不是根元素  根元素对应的level_stack_==0
                Level* level = level_stack_.template Top<Level>();// 返回当前栈顶(当前JSON层级)的Level对象
                if(level->valueCount > 0){// 当前嵌套层级已经包含元素,因此需要在新元素前添加合适的符合
                    if(level->inArray)// 当前层级是数组且非第一个元素,则写入新值前要添加逗号
                        os_->Put(',');
                    else
                        os_->Put((level->valueCount % 2 ==0) ? ',':':');// 当前层级是对象,且valueCount是偶数则添加逗号(表示一个新键值对的开始),否则添加冒号(表示键与值之间的分隔)
                }
                if(!level->inArray&&level->valueCount%2==0)// 当前层级是对象,并且valueCount为偶数(表示一个新键值对的开始)
                    RAPIDJSON_ASSERT(type == kStringType);// JSON对象的键值必须为字符串,因此这里做了一个断言检查
                level->valueCount++;
            }
            else{
                RAPIDJSON_ASSERT(!hasRoot_);
                hasRoot_ = true;// 根元素已经存在了
            }
        }
        bool EndValue(bool ret){
            // 写完一个数组/对象就会EndArray()/EndObject()(里面有Pop())
            if(RAPIDJSON_UNLIKELY(level_stack_.Empty()))// level_stack_为空,说明达到了JSON文本的结束  此时将输出流数据Flush()
                Flush();
            return ret;
        }
        OutputStream* os_;// 指向输出流的指针
        internal::Stack<StackAllocator> level_stack_;
        int maxDecimalPlaces_;// 表示数字序列化时允许的小数位数
        bool hasRoot_;// 标记是否已有根元素
    private:
        Writer(const Writer&) = delete; 
        Writer& operator=(const Writer&) = delete;
    };   
    /**
     * @brief 负责将各种基本数据类型(int、unsigned、int64_t、uint64_t、double)写入StringBuffer.具体来说,它们将这些数据类型转化为字符串表示形式并将其写入缓冲区
     * 下面这些函数都是Writer中输出流为StringBuffer的特化版本
     * @param  
     * @param i 
     * @return true 
     * @return false 
     */
    template<>
    inline bool Writer<StringBuffer>::WriteInt(int i){
        char* buffer = os_->Push(11);// 直接在输出流的缓冲区上申请11个字节空间(这样后续就不用再把转换后的字符串采用前面通用版本中逐字符写入的方法了),用于存放最多10位数的int值(包含负号)
        const char* end = internal::i32toa(i, buffer);// int->string,并存放于buffer
        os_->Pop(static_cast<size_t>(11-(end-buffer)));// 利用Pop来调整缓冲区大小,将Push进来的多余空间移除
        return true;
    } 
    template<>
    inline bool Writer<StringBuffer>::WriteUint(unsigned u) {
        char *buffer = os_->Push(10);// 无符号
        const char* end = internal::u32toa(u, buffer);
        os_->Pop(static_cast<size_t>(10 - (end - buffer)));
        return true;
    }
    template<>
    inline bool Writer<StringBuffer>::WriteInt64(int64_t i64) {
        char *buffer = os_->Push(21);// 直接在输出流缓冲区上申请空间
        const char* end = internal::i64toa(i64, buffer);
        os_->Pop(static_cast<size_t>(21 - (end - buffer)));
        return true;
    }
    template<>
    inline bool Writer<StringBuffer>::WriteUint64(uint64_t u) {
        char *buffer = os_->Push(20);// 直接在输出流缓冲区上申请空间
        const char* end = internal::u64toa(u, buffer);
        os_->Pop(static_cast<size_t>(20 - (end - buffer)));
        return true;
    }
    template<>
    inline bool Writer<StringBuffer>::WriteDouble(double d){
        if(internal::Double(d).IsNanOrInf()){// 处理Nan或Inf的情况
            if(!(kWriteNanAndInfFlag)&&!(kWriteNanAndInfNullFlag))
                return false;
            if(kWriteDefaultFlags&kWriteNanAndInfNullFlag){// 启用了kWriteNanAndInfNullFlag,则将Nan处理位JSON的null,并向输出流写入null
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
                PutUnsafe(*os_, '-');
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
        char* buffer = os_->Push(25);// 为double最大表示的字符串形式申请25字节空间
        char* end = internal::dtoa(d, buffer, maxDecimalPlaces_);
        os_->Pop(static_cast<size_t>(25-(end-buffer)));
        return true;
    }
    /**
     * @brief 特化版本:如果定义了RAPIDJSON_SSE2 或 RAPIDJSON_SSE42,则使用 SSE 指令集优化代码
     * 基于SIMD优化,用于快速扫描和写入未转义的JSON字符串(即写入没有转义字符的部分),它通过SSE指令集批量处理字符串中的字符
     * 遇到特殊字符就会将此位置赋给输入流的is.src_,然后结束函数
     * @param  
     * @param is 
     * @param length 
     * @return true 
     * @return false 
     */
    #if defined(RAPIDJSON_SSE2) || defined(RAPIDJSON_SSE42)
    template<>
    inline bool Writer<StringBuffer>::ScanWriteUnescapedString(StringStream& is, size_t length){
        if(length < 16)// 若字符串长度小于16字节,则直接返回当前字符位置是否小于长度length
            return RAPIDJSON_LIKELY(is.Tell() < length);
        if(!RAPIDJSON_LIKELY(is.Tell() < length))// 当前位置超出指定长度
            return false;
        const char* p =is.src_;
        const char* end = is.head_ + length;// 字符串结束位置
        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址    向上对齐
        const char* endAligned = reinterpret_cast<const char*>(reinterpret_cast<size_t>(end) & static_cast<size_t>(~15));// end末尾对应的16字节对齐的地址,这样就可以指定SIMD批处理的终止位置  向下对齐
        if(nextAligned > end)// 表示此时剩余数据不足16字节
            return true;
        // 未对齐部分逐字符单独处理   不用SSE处理
        while(p!=nextAligned){// p!=nextAligned表示此时是未到16字节对齐的地址的  即未对齐
            if(*p < 0x20 || *p=='\"' || *p=='\\'){// 如果是控制字符、双引号或反斜杠=>特殊字符
                is_.src_ = p;
                return RAPIDJSON_LIKELY(is.Tell() < length);// 若遇到特殊字符,返回当前位置
            }
            else
                os_->PutUnsafe(*p++);// 不是特殊字符,则直接写入输出流
        }
        // 定义SIMD常量
        static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };// 双引号
        static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };// 反斜杠
        static const char space[16]  = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };// 控制字符上限  即小于0x1F的都是控制字符
        const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&dquote[0]));// 加载dquote这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
        const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
        const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));
        // 对16字节对齐的部分进行SSE批量处理 即一次处理16字节
        for(; p!=endAligned;p+=16){// 每次读取16个字节的数据块
            const __128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(p));// 从p加载16字节数据到寄存器s中
            const __m128i t1 = _mm_cmpeq_epi8(s, dq);// 逐字节比较比较是否为双引号
            const __m128i t2 = _mm_cmpeq_epi8(s, bs);// 逐字节比较比较是否为反斜杠
            const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp);// 逐字节比较比较是否为控制字符
            const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);// 合并比较结果
            unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8)(x));// 生成掩码  如果掩码r中某位=1,则表示对应的字符是特殊字符
            if(RAPIDJSON_UNLIKELY(r!=0)){// r不为0,则表示合并的结果中出现了特殊字符
                SizeType len;
                len = static_cast<SizeType>(__builtin_ffs(r) - 1);// 查找第一个非零位 则非零位前都是无转义字符
                char* q = reinterpret_cast<char*>(os_->PushUnsafe(len));
                for(size_t i=0;i<len;++i)
                    q[i] = p[i];// 拷贝无转义字符的部分 len之前的是普通字符
                p += len;// 更新p位置以跳过已处理的普通字符部分
                break;// 当扫描到特殊字符,就停止扫描,即退出函数
            }
            _mm_storeu_si128(reinterpret_cast<__m128i*>(os_->PushUnsafe(16), s);// 无特殊字符,则直接写入输出流
        }
        is.src_ = p;
        return RAPIDJSON_LIKELY(is.Tell() < length);// 返回流当前位置小于总长度,用于后续检测是否达到结束
    }
    #elif defined(RAPIDJSON_NEON)
    template<>
    inline bool Writer<StringBuffer>::ScanWriteUnescapedString(StringStream& is, size_t length) {
      if(length < 16)// 若字符串长度小于16字节,则直接返回当前字符位置是否小于长度length
            return RAPIDJSON_LIKELY(is.Tell() < length);
        if(!RAPIDJSON_LIKELY(is.Tell() < length))// 当前位置超出指定长度
            return false;
        const char* p =is.src_;
        const char* end = is.head_ + length;// 字符串结束位置
        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址
        const char* endAligned = reinterpret_cast<const char*>(reinterpret_cast<size_t>(end) & static_cast<size_t>(~15));// end末尾对应的16字节对齐的地址,这样就可以指定SIMD批处理的终止位置
        if(nextAligned > end)// 表示此时剩余数据不足16字节
            return true;
        // 未对齐部分逐字符单独处理   不用SSE处理
        while(p!=nextAligned){// p!=nextAligned表示此时是未到16字节对齐的地址的  即未对齐
            if(*p < 0x20 || *p=='\"' || *p=='\\'){// 如果是控制字符、双引号或反斜杠=>特殊字符
                is_.src_ = p;
                return RAPIDJSON_LIKELY(is.Tell() < length);// 若遇到特殊字符,返回当前位置
            }
            else
                os_->PutUnsafe(*p++);// 不是特殊字符,则直接写入输出流
        }
        // 定义NEON常量
        const uint8x16_t s0 = vmovq_n_u8('"');// 使用vmovq_n_u8加载双引号到NEON寄存器
        const uint8x16_t s1 = vmovq_n_u8('\\');// 使用vmovq_n_u8加载反斜杠到NEON寄存器
        const uint8x16_t s2 = vmovq_n_u8('\b');// 使用vmovq_n_u8加载退格符到NEON寄存器   NEON指令集中对退格符\b单独进行检测了,而不是纳入控制字符中被统一检测
        const uint8x16_t s3 = vmovq_n_u8(32);// 使用vmovq_n_u8加载控制字符到NEON寄存器  32<=>0x20
       for(;p!=endAligned;p+=16){
            const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t*>(p));
            uint8x16_t x = vceqq_u8(s, s0);// 使用vceqq_u8比较字符是否为特殊字符s0
            x = vorrq_u8(x, vceqq_u8(s, s1));// 使用vceqq_u8比较字符是否为特殊字符s1
            x = vorrq_u8(x, vceqq_u8(s, s2));// 使用vceqq_u8比较字符是否为特殊字符s2
            x = vorrq_u8(x, vcltq_u8(s, s3));// 使用vceqq_u8比较字符是否小于控制字符界限s3(小于控制字符界限就说明它是控制字符)
            x = vrev64q_u8(x);// 对x进行64位反转(前8字节和后8字节交换)
            uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);// 反转后的64位低位,代表检测结果的掩码
            uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);// 反转后的64位高位,代表检测结果的掩码
            SizeType len = 0;
            bool escaped = false;// 标记是否检测到特殊字符
            // 计算16字节块中特殊字符的索引,即算无特殊字符部分的长度len
            // SSE指令集用__builtin_ffs()就能完成下面if-else中len1的计算
           if(low==0){// low为全零,表示低位无特殊字符
                if(high!=0){// high非零,则通过clzll计算零位数,即算无特殊字符部分的长度
                    uint32_t lz = internal::clzll(high);
                    len = 8+(lz>>3);
                    escaped = true;
                }
            }
            else{// low非零,则通过clzll计算零位数,即算无特殊字符部分的长度
                uint32_t lz = internal::clzll(low);
                len = lz>>3;
                escaped = true;
            }
            if(RAPIDJSON_UNLIKELY(escaped)){// 写入无特殊字符的部分并跳出
                char* q = reinterpret_cast<char*>(os_->PushUnsafe(len));
                for(size_t i=0;i<len;++i)
                    q[i] = p[i];
                p += len;
                break;
            }
            vst1q_u8(reinterpret_cast<uint8_t *>(os_->PushUnsafe(16)), s);// 这个16字节块无特殊字符 则直接写入
        }
        is.src_ = p;
        return RAPIDJSON_LIKELY(is.Tell() < length);
    }
    #endif
}
#endif