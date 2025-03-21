// JSON字符串->JSON数据
#ifndef RAPIDJSON_READER_H_
#define RAPIDJSON_READER_H_

#include "allocators.h"
#include "stream.h"
#include "encodedstream.h"
#include "internal/clzll.h"
#include "internal/meta.h"
#include "internal/stack.h"
#include "internal/strtod.h"
#include <limits>

// SSE指令集用于提供一系列操作 用于加速向量和矩阵计算
#ifdef RAPIDJSON_SSE42
#include <nmmintrin.h>
#elif defined(RAPIDJSON_SSE2)
#include <emmintrin.h>
#elif defined(RAPIDJSON_NEON)
#include <arm_neon.h>
#endif

//!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
// 定义一个空宏,用于特定目的,避免编译器报错
#define RAPIDJSON_NOTHING /* deliberately empty */

// 如果没有定义 RAPIDJSON_PARSE_ERROR_EARLY_RETURN
// 定义一个宏，用于在解析过程中检测解析错误
// RAPIDJSON_PARSE_ERROR_EARLY_RETURN(value)<=>if (RAPIDJSON_UNLIKELY(HasParseError())) { return value; }
#ifndef RAPIDJSON_PARSE_ERROR_EARLY_RETURN
#define RAPIDJSON_PARSE_ERROR_EARLY_RETURN(value) \
    RAPIDJSON_MULTILINEMACRO_BEGIN \  
    if (RAPIDJSON_UNLIKELY(HasParseError())) { return value; } \
    RAPIDJSON_MULTILINEMACRO_END
#endif

// 定义一个宏,用于在解析错误时返回空值
#define RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID \
    RAPIDJSON_PARSE_ERROR_EARLY_RETURN(RAPIDJSON_NOTHING) // 调用前一个宏，传入空值
//!@endcond

// 如果没有定义 RAPIDJSON_PARSE_ERROR_NORETURN
#ifndef RAPIDJSON_PARSE_ERROR_NORETURN
// 定义一个宏，用于处理解析错误，不返回值
// parseErrorCode在error.h中定义,表示不同的错误形式  offset指示在原始JSON字符串中的位置
#define RAPIDJSON_PARSE_ERROR_NORETURN(parseErrorCode, offset) \
    RAPIDJSON_MULTILINEMACRO_BEGIN \
    RAPIDJSON_ASSERT(!HasParseError()); \
    SetParseError(parseErrorCode, offset); \
    RAPIDJSON_MULTILINEMACRO_END
#endif

// 如果没有定义 RAPIDJSON_PARSE_ERROR
#ifndef RAPIDJSON_PARSE_ERROR
// 定义一个宏，用于处理解析错误并返回
#define RAPIDJSON_PARSE_ERROR(parseErrorCode, offset) \
    RAPIDJSON_MULTILINEMACRO_BEGIN \ 
    RAPIDJSON_PARSE_ERROR_NORETURN(parseErrorCode, offset); \
    RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID; \
    RAPIDJSON_MULTILINEMACRO_END 
#endif

// 包含错误处理相关的头文件
#include "error/error.h" // ParseErrorCode, ParseResult

namespace RAPIDJSON{
    #ifndef RAPIDJSON_PARSE_DEFAULT_FLAGS
    #define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseNoFlags
    #endif
    enum ParseFlag{
        kParseNoFlags = 0,// 不设置任何标志,表示默认解析行为
        kParseInsituFlag = 1,// 在原位解析,直接在输入缓冲区上修改数据,这样更高效,但是会改变原始数据
        kParseValidateEncodingFlag = 2,// 验证JSON字符串的编码格式
        kParseIterativeFlag = 4,// 采用迭代解析方式,避免递归调用以减少栈的复杂度
        kParseStopWhenDoneFlag = 8,// 当解析完整的JSON根节点后,停止解析,忽略剩余内容
        kParseFullPrecisionFlag = 16,// 以完整精度解析数字,但会稍微影响性能
        kParseCommentsFlag = 32,// 允许解析JSON中的注释
        kParseNumbersAsStringsFlag = 64,// 将所有数字解析为字符串,而非数字类型
        kParseTrailingCommasFlag = 128,// 允许对象或数组末尾存在多余的逗号
        kParseNanAndInfFlag = 256,// 允许将NaN、Inf、-Inf等值解析为double类型
        kParseEscapedApostropheFlag = 512,// 允许在字符串中使用转义的单引号'\'
        kParseDefaultFlags = RAPIDJSON_PARSE_DEFAULT_FLAGS// 使用默认解析标志
    };
    /*
    \brief JSON的解析接口函数Handler  最简单的是可以直接让每个接口函数直接输出解析得到的JSON数据
    \code
    concept Handler {
        typename Ch;
        bool Null();
        bool Bool(bool b);
        bool Int(int i);
        bool Uint(unsigned i);
        bool Int64(int64_t i);
        bool Uint64(uint64_t i);
        bool Double(double d);
        bool RawNumber(const Ch* str, SizeType length, bool copy);
        bool String(const Ch* str, SizeType length, bool copy);
        bool StartObject();
        bool Key(const Ch* str, SizeType length, bool copy);
        bool EndObject(SizeType memberCount);
        bool StartArray();
        bool EndArray(SizeType elementCount);
    };
    \endcode
    */
    /**
     * @brief BaseReaderHandler 提供了一套默认的 JSON 数据解析处理接口,并通过 Override 机制支持派生类的重载
     * 
     * @tparam Encoding 
     * @tparam Derived 
     */
    template<typename Encoding=UTF8<>, typename Derived=void>// Derived为派生类类型
    struct BaseReaderHandler{
        typedef typename Encoding::Ch Ch;
        typedef typename internal::SelectIf<internal::IsSame<Derived, void>, BaseReaderHandler, Derived>::Type Override;
        bool Default() {return true;}
        // 下面函数中static_cast<Override&>(*this).Default()调用Default()来进行默认处理,但支持动态重载.即若下面这些解析函数未被外部重载,那么就会执行默认的Default()
        bool Null() {return static_cast<Override&>(*this).Default();}
        bool Bool(bool) { return static_cast<Override&>(*this).Default(); }
        bool Int(int) { return static_cast<Override&>(*this).Default(); }
        bool Uint(unsigned) { return static_cast<Override&>(*this).Default(); }
        bool Int64(int64_t) { return static_cast<Override&>(*this).Default(); }
        bool Uint64(uint64_t) { return static_cast<Override&>(*this).Default(); }
        bool Double(double) { return static_cast<Override&>(*this).Default(); }
        bool RawNumber(const Ch* str, SizeType len, bool copy) { return static_cast<Override&>(*this).String(str, len, copy);}// 在启用kParseNumbersAsStringsFlag时被调用,传入的是一个输入流的原始数值字符串
        bool String(const Ch*, SizeType, bool) { return static_cast<Override&>(*this).Default(); }
        bool StartObject() { return static_cast<Override&>(*this).Default(); }
        bool Key(const Ch* str, SizeType len, bool copy) { return static_cast<Override&>(*this).String(str, len, copy); }
        bool EndObject(SizeType) { return static_cast<Override&>(*this).Default(); }
        bool StartArray() { return static_cast<Override&>(*this).Default(); }
        bool EndArray(SizeType) { return static_cast<Override&>(*this).Default(); } 
    };
    namespace internal{
        template<typename Stream, int = StreamTraits<Stream>::copyOptimization>
        class StreamLocalCopy;// 通用模板(啥也不做)
        template<typename Stream>
        class StreamLocalCopy<Stream, 1>{// 特化模板 使用拷贝优化
            public:
                StreamLocalCopy(Stream& original) : s(original), original_(original) {}
                ~StreamLocalCopy() {original_ = s;}// 析构时在对象销毁时将副本的状态更新回original,确保原流的状态保存一致
                Stream s;// 非引用类型,因此会创建副本
            private:
                StreamLocalCopy& operator=(const StreamLocalCopy&) = delete;
                Stream& original_;      
        };
        template<typename Stream>
        class StreamLocalCopy<Stream, 0>{// 特化模板 不使用拷贝优化
            public:
                StreamLocalCopy(Stream& original) : s(original){}
                Stream& s;// 非引用类型,因此会创建副本
            private:
                StreamLocalCopy& operator=(const StreamLocalCopy&) = delete;     
        };
    }
        /**
         * @brief 用于在JSON解析时跳过空白字符(空格、换行、回车、制表符)
         * 
         * @tparam InputStream 
         * @param is 
         */
        template<typename InputStream>
        void SkipWhitespace(InputStream& is){
            internal::StreamLocalCopy<InputStream> copy(is);
            InputStream& s(copy.s);
            typename InputStream::Ch c;
            // 使用 Peek() 检查下一个字符是否为空白字符(空格、换行、回车、制表符),如果是则调用 Take() 移动到下一个字符,跳过空白字符;如果不是则此函数啥也不做
            while((c=s.Peek())==' '||c=='\n'||c=='\r'||c=='\t')
                s.Take();
        }
        /**
         * @brief 指针版本的输入流,即输入流为p和end指针表示.通过指针来找到非空白字符,并返回
         * 
         * @param p 
         * @param end 
         * @return const char* 
         */
        inline const char* SkipWhitespace(const char* p, const char* end){
            while(p!=end&&(*p==' '||*p=='\n'||*p=='\r'||*p=='\t'))
                ++p;
            return p;// 返回传入的p至end之间指向第一个非空白字符的指针
        }
        #ifdef RAPIDJSON_SSE42
        /**
         * @brief 通过SIMD加速跳过一段字符串中的空白字符,使得解析器能更快找到下一个有效字符
         * 找到非空白字符就立即返回函数
         * @param p 
         * @return const char* 
         */
        inline const char *SkipWhitespace_SIMD(const char* p){
            if(*p==' '||*p=='\n'||*p=='\r'||*p=='\t')
                ++p;
            else
                return p;
            // 对齐内存 16字节边界
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址
            while (p != nextAligned){// 如果 p 尚未到达 nextAligned,则逐个检查字符,直到达到对齐位置或找到非空白字符为止
                if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
                    ++p;
                else
                    return p;
            }
            static const char whitespace[16] = "\n\r\t";// 空白字符集合
            const __m128i w = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[0]));// 加载whitespace这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            for(;;p+=16){
                const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(p));// 从p加载16字节数据到寄存器s中
                const int r = _mm_cmpistri(w, s, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT | _SIDD_NEGATIVE_POLARITY);// 按照指定的四种模式下进行比较空白字符集合w和当前指针位置的16个字符s
                if(r!=16)// 发现了空白字符
                    return p+r;
            }
        }
        inline const char *SkipWhitespace_SIMD(const char* p, const char* end){// 此时不用在首部p进行16字节边界对齐,只是很可能会在后面剩小于16字节的字符还需SkipWhitespace(p, end)单独处理
            if(p != end && (*p==' '||*p=='\n'||*p=='\r'||*p=='\t'))
                ++p;
            else
                return p;
            static const char whitespace[16] = "\n\r\t";// 空白字符集合
            const __m128i w = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[0]));// 加载whitespace这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            for(;p<=end-16;p+=16){// p<=end-16保证每次都可以处理到16字节一批的数据
                const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(p));// 从p加载16字节数据到寄存器s中
                const int r = _mm_cmpostri(w, s, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT | _SIDD_NEGATIVE_POLARITY);// 按照指定的四种模式下进行比较空白字符集合w和当前指针位置的16个字符s
                if(r!=16)// 发现了空白字符
                    return p+r;
            }
            return SkipWhitespace(p, end);// 未找到非空白字符,则调用SkipWhitespace(p, end)继续逐个字符检查剩余内容(因为此时可能p与end相差16个字节以内,因此还可能有剩余字符)
        }
        #elif defined(RAPIDJSON_SSE2)
        inline const char *SkipWhitespace_SIMD(const char* p){
            if(*p==' '||*p=='\n'||*p=='\r'||*p=='\t')
                ++p;
            else
                return p;
            // 对齐内存 16字节边界
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址
            while (p != nextAligned){// 如果 p 尚未到达 nextAligned,则逐个检查字符,直到达到对齐位置或找到非空白字符为止
                if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
                    ++p;
                else
                    return p;
            }
            #define C16(c) {c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c}
            static const char whitespaces[4][16] = {C16(' '), C16('\n'), C16('\r'), C16('\t')))};
            #undef C16
            const __m128i w0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[0][0]));// 加载whitespace[0]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            const __m128i w1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[1][0]));// 加载whitespace[1]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            const __m128i w2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[2][0]));// 加载whitespace[2]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            const __m128i w3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[3][0]));// 加载whitespace[3]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            for(;;p+=16){
                const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(p));// 从p加载16字节数据到寄存器s中
                __m128i x0 = _mm_cmpeq_epi8(s, w0);// 比较 s 和 w0,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x1 = _mm_cmpeq_epi8(s, w1);// 比较 s 和 w1,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x2 = _mm_cmpeq_epi8(s, w2);// 比较 s 和 w2,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x3 = _mm_cmpeq_epi8(s, w3);// 比较 s 和 w3,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x = _mm_or_si128(_mm_or_si128(x0, x1), _mm_or_si128(x2, x3));// 合并比较结果
                unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8)(x));// 生成掩码  如果掩码r中某位=1,则表示对应的字符是空白字符
                if(r!=0)// 出现了空白字符
                    return p + __builtin_ffs(r) - 1;// 查找第一个非空白字符的位置并返回
            }
        }
        inline const char *SkipWhitespace_SIMD(const char* p, const char* end){// 此时不用在首部p进行16字节边界对齐,只是很可能会在后面剩小于16字节的字符还需SkipWhitespace(p, end)单独处理
            if(p != end && (*p==' '||*p=='\n'||*p=='\r'||*p=='\t'))
                ++p;
            else
                return p;
           #define C16(c) {c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c}
            static const char whitespaces[4][16] = {C16(' '), C16('\n'), C16('\r'), C16('\t')))};
            #undef C16
            const __m128i w0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[0][0]));// 加载whitespace[0]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            const __m128i w1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[1][0]));// 加载whitespace[1]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            const __m128i w2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[2][0]));// 加载whitespace[2]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            const __m128i w3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&whitespace[3][0]));// 加载whitespace[3]这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
            for(;p<=end-16;p+=16){
                const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(p));// 从p加载16字节数据到寄存器s中
                __m128i x0 = _mm_cmpeq_epi8(s, w0);// 比较 s 和 w0,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x1 = _mm_cmpeq_epi8(s, w1);// 比较 s 和 w1,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x2 = _mm_cmpeq_epi8(s, w2);// 比较 s 和 w2,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x3 = _mm_cmpeq_epi8(s, w3);// 比较 s 和 w3,结果保存到 x 中.如果s中的字符是空格,对应的x位会置为1
                __m128i x = _mm_or_si128(_mm_or_si128(x0, x1), _mm_or_si128(x2, x3));// 合并比较结果
                unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8)(x));// 生成掩码  如果掩码r中某位=1,则表示对应的字符是空白字符
                if(r!=0)// 出现了空白字符
                    return p + __builtin_ffs(r) - 1;// 查找第一个非空白字符的位置并返回
            }
            return SkipWhitespace(p, end);
        }
        #elif defined(RAPIDJSON_NEON)
        inline const char *SkipWhitespace_SIMD(const char* p){
            if(*p==' '||*p=='\n'||*p=='\r'||*p=='\t')
                ++p;
            else
                return p;
            // 对齐内存 16字节边界
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址
            while (p != nextAligned){// 如果 p 尚未到达 nextAligned,则逐个检查字符,直到达到对齐位置或找到非空白字符为止
                if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
                    ++p;
                else
                    return p;
            }
            // 定义NEON常量
            const uint8x16_t s0 = vmovq_n_u8(' ');// 使用vmovq_n_u8加载空格到NEON寄存器
            const uint8x16_t s1 = vmovq_n_u8('\n');// 使用vmovq_n_u8加载换行符到NEON寄存器
            const uint8x16_t s2 = vmovq_n_u8('\r');// 使用vmovq_n_u8加载'\r'到NEON寄存器
            const uint8x16_t s3 = vmovq_n_u8('\t');// 使用vmovq_n_u8加载制表符到NEON寄存器
            for(;;p+=16){
                const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t*>(p));// 从p加载16字节数据到寄存器s中
                uint8x16_t x = vceqq_u8(s, s0);// 使用vceqq_u8比较字符是否为空白字符s0
                x = vorrq_u8(x, vceqq_u8(s, s1));// 使用vceqq_u8比较字符是否为空白字符s1
                x = vorrq_u8(x, vceqq_u8(s, s2));// 使用vceqq_u8比较字符是否为空白字符s2
                x = vorrq_u8(x, vceqq_u8(s, s3));// 使用vceqq_u8比较字符是否小于空白字符s3
                x = vmvnq_u8(x);// 按位取反
                x = vrev64q_u8(x);// 对x进行64位字节反转并提取每8位的结果,使高8字节和低8字节互换
                uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);// 反转后的64位低位,代表检测结果的掩码
                uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);// 反转后的64位高位,代表检测结果的掩码
                if(low==0){// low为全零,表示低位无空白字符
                    if(high!=0){// high非零,则通过clzll计算零位数,即算无空白字符部分的长度
                        uint32_t lz = internal::clzll(high);
                        return p+8+(lz>>3);// 高8字节出现空白字符,所以这里要+8  lz>>3<=>lz/8(因为p指针直接加减的是字节)
                    }
                }
                else{// low非零,则低8字节中出现空白字符,通过clzll计算零位数,即算无特殊字符部分的长度
                    uint32_t lz = internal::clzll(low);
                    return p+(lz>>3);
                }
            }
        }
        inline const char *SkipWhitespace_SIMD(const char* p, const char* end){// 此时不用在首部p进行16字节边界对齐,只是很可能会在后面剩小于16字节的字符还需SkipWhitespace(p, end)单独处理
            if(p != end && (*p==' '||*p=='\n'||*p=='\r'||*p=='\t'))
                ++p;
            else
                return p;
           // 定义NEON常量
            const uint8x16_t s0 = vmovq_n_u8(' ');// 使用vmovq_n_u8加载空格到NEON寄存器
            const uint8x16_t s1 = vmovq_n_u8('\n');// 使用vmovq_n_u8加载换行符到NEON寄存器
            const uint8x16_t s2 = vmovq_n_u8('\r');// 使用vmovq_n_u8加载'\r'到NEON寄存器
            const uint8x16_t s3 = vmovq_n_u8('\t');// 使用vmovq_n_u8加载制表符到NEON寄存器
            for(;p<=end-16;p+=16){
                const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t*>(p));// 从p加载16字节数据到寄存器s中
                uint8x16_t x = vceqq_u8(s, s0);// 使用vceqq_u8比较字符是否为空白字符s0
                x = vorrq_u8(x, vceqq_u8(s, s1));// 使用vceqq_u8比较字符是否为空白字符s1
                x = vorrq_u8(x, vceqq_u8(s, s2));// 使用vceqq_u8比较字符是否为空白字符s2
                x = vorrq_u8(x, vceqq_u8(s, s3));// 使用vceqq_u8比较字符是否小于空白字符s3
                x = vmvnq_u8(x);// 按位取反
                x = vrev64q_u8(x);// 对x进行64位反转(前8字节和后8字节交换)
                uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);// 反转后的64位低位,代表检测结果的掩码
                uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);// 反转后的64位高位,代表检测结果的掩码
                if(low==0){// low为全零,表示低位无特殊字符
                    if(high!=0){// high非零,则通过clzll计算零位数,即算无特殊字符部分的长度
                        uint32_t lz = internal::clzll(high);
                        return p+8+(lz>>3);
                    }
                }
                else{// low非零,则通过clzll计算零位数,即算无特殊字符部分的长度
                    uint32_t lz = internal::clzll(low);
                    return p+(lz>>3);
                }
            }
            return SkipWhitespace(p, end);
        }
        #endif
        #ifdef RAPIDJSON_SIMD// 指定SIMD指令集时才能在SkipWhitespace调用SkipWhitespace_SIMD()
        template<> inline void SkipWhitespace(InsituStringStream& is) {// 原地解析流
            is.src_ = const_cast<char*>(SkipWhitespace_SIMD(is.src_));
        }
        template<> inline void SkipWhitespace(StringStream& is) {
            is.src_ = SkipWhitespace_SIMD(is.src_);
        }
        template<> inline void SkipWhitespace(EncodedInputStream<UTF8<>, MemoryStream>& is) {
            is.is_.src_ = SkipWhitespace_SIMD(is.is_.src_, is.is_.end_);
        }
        #endif // RAPIDJSON_SIMD
        /**
         * @brief 用于将JSON文本解析为树状结构的对象
         * 
         * @tparam SourceEncoding 
         * @tparam TargetEncoding 
         * @tparam StackAllocator 
         */
        template<typename SourceEncoding, typename TargetEncoding, typename StackAllocator=CrtAllocator>// 输入流编码、输出流编码、栈分配器
        class GenericReader{
            public:
                typedef typename SourceEncoding::Ch Ch;
                GenericReader(StackAllocator* allocator=nullptr, size_t stackCapacity = kDefaultStackCapacity)
                    : stack_(allocator, stackCapacity),// 栈用于存储临时数据,如解析过程中需要的字符或字符串
                      parseResult_(),// 存储解析结果,如是否成功和错误信息
                      state_(IterativeParsingStartState)// 保存当前的解析状态  来标志当前解析的是啥,比如:键解析状态
                    {}
                /**
                 * @brief 用于解析JSON文本,并根据parseFlags标志进行不同的处理
                 * 
                 * @tparam parseFlags 
                 * @tparam InputStream 
                 * @tparam Handler 
                 * @param is 
                 * @param handler 
                 * @return ParseResult 
                 */
                template<unsigned parseFlags, typename InputStream, typename Handler>
                ParseResult Parse(InputStream& is, Handler& handler){// Handler指的是BaseReaderHandler这种,可以自定义解析函数
                    if(parseFlags&kParseIterativeFlag)// 迭代解析
                        return IterativeParse<parseFlags>(is, handler);
                    parseResult_.Clear();
                    ClearStackOnExit scope(*this);// 确保在解析完成后栈被清空,防止内存泄漏
                    SkipWhitespaceAndComments<parseFlags>(is);// 跳过输入流中的空白字符和注释
                    RAPIDJSON_PARSE_ERROR_EARLY_RETURN(parseResult_);// 检查是否出现错误,若出现错误就立刻返回
                    if (RAPIDJSON_UNLIKELY(is.Peek() == '\0')) {// 检查JSON文档是否为空,为空就抛出kParseErrorDocumentEmpty错误
                        RAPIDJSON_PARSE_ERROR_NORETURN(kParseErrorDocumentEmpty, is.Tell());
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
                    }
                    else {// 不为空就调用ParseValue开始解析JSON文本
                        ParseValue<parseFlags>(is, handler);
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
                        if (!(parseFlags & kParseStopWhenDoneFlag)) {// 当 kParseStopWhenDoneFlag 标志位被设置时:解析器会在解析第一个完整的根对象后停止,不会继续向后检查内容.这意味着允许单一根对象解析完成后立即结束,不会报错;当未被设置时解析器会在解析完第一个根对象后继续读取剩余字符,检查是否文档根元素是唯一的
                            SkipWhitespaceAndComments<parseFlags>(is);
                            RAPIDJSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
                            if (RAPIDJSON_UNLIKELY(is.Peek() != '\0')) {// 检查JSON文档的根元素是否唯一  如果此时流中还有剩余字符那么就有多个根元素
                                RAPIDJSON_PARSE_ERROR_NORETURN(kParseErrorDocumentRootNotSingular, is.Tell());
                                RAPIDJSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
                            }
                        }
                    }
                    return parseResult_;
                }
                /**
                 * @brief 带有默认解析标志的Parse()
                 * 
                 * @tparam InputStream 
                 * @tparam Handler 
                 * @param is 
                 * @param handler 
                 * @return ParseResult 
                 */
                template <typename InputStream, typename Handler>
                ParseResult Parse(InputStream& is, Handler& handler) {
                    return Parse<kParseDefaultFlags>(is, handler);
                }
                /**
                 * @brief 初始化迭代解析器
                 * 
                 */
                void IterativeParseInit(){
                    parseResult_.Clear();
                    state_ = IterativeParsingStartState;
                }
                /**
                 * @brief 迭代模式下的逐步解析
                 * 
                 * @tparam parseFlags 
                 * @tparam InputStream 
                 * @tparam Handler 
                 * @param is 
                 * @param handler 
                 * @return true 
                 * @return false 
                 */
                template <unsigned parseFlags, typename InputStream, typename Handler>
                bool IterativeParseNext(InputStream& is, Handler& handler) {
                    while (RAPIDJSON_LIKELY(is.Peek() != '\0')) {// 不为空JSON文档
                        SkipWhitespaceAndComments<parseFlags>(is);
                        Token t = Tokenize(is.Peek());// 将当前字符转换成Token标记类型  用于算解析状态
                        IterativeParsingState n = Predict(state_, t);// 根据当前状态state_和标记t来预测下一个解析状态
                        IterativeParsingState d = Transit<parseFlags>(state_, t, n, is, handler);// 这是实际的解析过程    根据当前状态 state_、标记 t、预测状态 n 以及输入流 is 和处理器 handler 来解析内容,解析完成后,状态结果保存在d
                        if (RAPIDJSON_UNLIKELY(IsIterativeParsingCompleteState(d))) {// 检查d是否是一个完成解析的状态
                            // 解析完成
                            if (d == IterativeParsingErrorState) {
                                HandleError(state_, is);
                                return false;
                            }
                            RAPIDJSON_ASSERT(d == IterativeParsingFinishState);
                            state_ = d;
                            if (!(parseFlags & kParseStopWhenDoneFlag)) {// 检查是否还有剩余内容  此时的剩余内容就表示JSON文档有多个根元素,返回错误
                                SkipWhitespaceAndComments<parseFlags>(is);
                                if (is.Peek() != '\0') {
                                    HandleError(state_, is);
                                    return false;
                                }
                            }
                            return true;
                        }
                        state_ = d;// 解析未完成,也更新状态  解析完成会在前面已经return了
                        if (!IsIterativeParsingDelimiterState(n))// 不是分隔符状态则直接返回true,此时没有解析完成,若要继续解析用户可以继续下一次迭代解析.此时可以在解析一个JSON片段后就返回,如解析一个字符串就可以返回了
                            return true;
                    }
                    stack_.Clear();// 解析完成就清空栈
                    if (state_ != IterativeParsingFinishState) {
                        HandleError(state_, is);
                        return false;
                    }
                    return true;
                }
                // 判断解析过程是否完成
                RAPIDJSON_FORCEINLINE bool IterativeParseComplete() const {
                    return IsIterativeParsingCompleteState(state_);
                }
                // 检查解析过程是否发生错误
                bool HasParseError() const { return parseResult_.IsError(); }
                // 获取解析错误代码
                ParseErrorCode GetParseErrorCode() const { return parseResult_.Code(); }
                // 获取解析错误的具体位置
                size_t GetErrorOffset() const { return parseResult_.Offset(); }
            protected:
                // 在解析过程中设置错误代码和错误位置  发生错误时会被调用,传入错误代码和错误位置
                void SetParseError(ParseErrorCode code, size_t offset) { parseResult_.Set(code, offset); }   
            private:
                GenericReader(const GenericReader&) = delete;
                GenericReader& operator=(const GenericReader&) = delete;
                void ClearStack() { stack_.Clear();}
                struct ClearStackOnExit{
                    explicit ClearStackOnExit(GenericReader& r) : r_(r) {}
                    ~ClearStackOnExit() {r_.ClearStack();}
                    private:
                        GenericReader& r_;
                        ClearStackOnExit(const ClearStackOnExit&) = delete;
                        ClearStackOnExit& operator=(const ClearStackOnExit&) = delete;
                };
                /**
                 * @brief 用于跳过输入流中的空白字符和注释  
                 * 单行注释://  多行注释
                 * @tparam parseFlags 
                 * @tparam InputStream 
                 * @param is 
                 */
                template<unsigned parseFlags, typename InputStream>
                void SkipWhitespaceAndComments(InputStream& is){
                    SkipWhitespace(is);// 跳过空白字符
                    if(parseFlags & kParseCommentsFlag){// 需要解析注释内容
                        while(RAPIDJSON_UNLIKELY(Consume(is, '/'))){
                            if(Consume(is, '*')){// 多行注释的开始  处理多行注释  /*
                                while(true){
                                    if(RAPIDJSON_UNLIKELY(is.Peek()=='\0'))// 到达文件末尾却没有找到注释结束符,则注释语法有错
                                        RAPIDJSON_PARSE_ERROR(kParseErrorUnspecificSyntaxError, is.Tell());
                                    else if(Consume(is, '*')){// 注释结束
                                        if(Consume(is, '/'))
                                            break;
                                    }
                                    else
                                        is.Take();// 一个一个读取跳过注释内容
                                }
                            }
                            else if(RAPIDJSON_LIKELY(Consume(is, '/')))// 单行注释的开始  处理单行注释 //
                                while(is.Peek()!='\0'&&is.Take()!='\n'){}// is.Peek()!='\0'是为了确保有内容  单行注释不能有换行符
                            else// 不正确的注释格式,可能写成了如:\/等形式
                                RAPIDJSON_PARSE_ERROR(kParseErrorUnspecificSyntaxError, is.Tell());
                            SkipWhitespace(is);// 在成功跳过注释后,再次调用SkipWhitespace,确保空白字符被完全跳过,使解析器可以准确地从下一个有效字符继续解析
                        }
                    }
                }
                /**
                 * @brief 解析JSON对象
                 * 
                 * @tparam parseFlags 
                 * @tparam InputStream 
                 * @tparam Handler 
                 * @param is 
                 * @param handler 
                 */
                template<unsigned parseFlags, typename InputStream, typename Handler>
                void ParseObject(InputStream& is, Handler& handler){
                    RAPIDJSON_ASSERT(is.Peek()=='{');
                    is.Take();// 跳过'{'
                    if(RAPIDJSON_UNLIKELY(!handler.StartObject()))
                        RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                    SkipWhitespaceAndComments<parseFlags>(is);// 跳过对象开始后的空白字符和注释
                    RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                    if(Consume(is, '}')){// 当前字符为},则表示对象为空
                        if(RAPIDJSON_UNLIKELY(!handler.EndObject(0)))
                            RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                        return;
                    }
                    for(SizeType memberCount=0;;){// 初始化对象的键值对成员为0
                        if(RAPIDJSON_UNLIKELY(is.Peek()!='"'))// 键必须是字符串
                            RAPIDJSON_PARSE_ERROR(kParseErrorObjectMissName, is.Tell());
                        ParseString<parseFlags>(is, handler, true);// 解析键  true表明这个字符串是对象中的键值对的键
                        
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;// 检查错误
                        SkipWhitespaceAndComments<parseFlags>(is);// 跳过空白字符和注释
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        if(RAPIDJSON_UNLIKELY(!Consume(is, ':')))// 如果键解析后的字符不是冒号,则报错
                            RAPIDJSON_PARSE_ERROR(kParseErrorObjectMissColon, is.Tell());
                        SkipWhitespaceAndComments<parseFlags>(is);
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        ParseValue<parseFlags>(is, handler);// 解析值
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        SkipWhitespaceAndComments<parseFlags>(is);
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        ++memberCount;
                        switch(is.Peek()){
                            case ',':// 一个键值对结束
                                is.Take();
                                SkipWhitespaceAndComments<parseFlags>(is);
                                RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                                break;
                            case '}':// 对象结束标志
                                is.Take();
                                if(RAPIDJSON_UNLIKELY(!handler.EndObject(memberCount)))// EndObject()执行失败
                                    RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                                return;
                            default:
                                RAPIDJSON_PARSE_ERROR(kParseErrorObjectMissCommaOrCurlyBracket, is.Tell());
                        }
                        if(parseFlags&kParseTrailingCommasFlag){// 处理尾随逗号,即设置了kParseTrailingCommasFlag,则允许对象最后一个成员后带有逗号
                            if(is.Peek()=='}'){
                                if (RAPIDJSON_UNLIKELY(!handler.EndObject(memberCount)))
                                    RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                                is.Take();
                                return;
                            }
                        }
                    }
                } 
                /**
                 * @brief 和ParseObject()基本一样  只是一个是对于对象,一个是对于数组
                 * 
                 * @tparam parseFlags 
                 * @tparam InputStream 
                 * @tparam Handler 
                 * @param is 
                 * @param handler 
                 */
                template<unsigned parseFlags, typename InputStream, typename Handler>
                void ParseArray(InputStream& is, Handler& handler){
                    RAPIDJSON_ASSERT(is.Peek()=='[');
                    is.Take();// 跳过'['
                    if(RAPIDJSON_UNLIKELY(!handler.StartArray()))
                        RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                    SkipWhitespaceAndComments<parseFlags>(is);// 跳过数组开始后的空白字符和注释
                    RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                    if(Consume(is, ']')){// 当前字符为],则表示数组为空
                        if(RAPIDJSON_UNLIKELY(!handler.EndArray(0)))
                            RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                        return;
                    }
                    for(SizeType elementCount=0;;){
                        ParseValue<parseFlags>(is, handler);// 解析数组元素
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        ++elementCount;
                        SkipWhitespaceAndComments<parseFlags>(is);
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        switch(is.Peek()){
                            case ',':// 一个数组元素结束
                                is.Take();
                                SkipWhitespaceAndComments<parseFlags>(is);
                                RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                                break;
                            case ']':
                                is.Take();
                                if(RAPIDJSON_UNLIKELY(!handler.EndArray(elementCount)))// EndObject()执行失败
                                    RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                                return;
                            default:
                                RAPIDJSON_PARSE_ERROR(kParseErrorArrayMissCommaOrSquareBracket, is.Tell());
                        }
                        if(parseFlags&kParseTrailingCommasFlag){// 处理尾随逗号,即设置了kParseTrailingCommasFlag,则允许数组最后一个元素后带有逗号
                            if(is.Peek()==']'){
                                if (RAPIDJSON_UNLIKELY(!handler.EndArray(elementCount)))
                                    RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                                is.Take();
                                return;
                            }
                        }
                    }
                }
                /**
                 * @brief 解析JSON中的Null
                 * 
                 * @tparam parseFlags 
                 * @tparam InputStream 
                 * @tparam Handler 
                 * @param is 
                 * @param handler 
                 */
                template<unsigned parseFlags, typename InputStream, typename Handler>
                void ParseNull(InputStream& is, Handler& handler){
                    RAPIDJSON_ASSERT(is.Peek()=='n');// null第一个为'n'
                    is.Take();
                    if(RAPIDJSON_LIKELY(Consume(is, 'u')&&Consume(is, 'l')&&Consume(is, 'l'))){
                        if(RAPIDJSON_UNLIKELY(!handler.Null()))
                            RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                    }
                    else
                        RAPIDJSON_PARSE_ERROR(kParseErrorValueInvalid, is.Tell());
                }
                // 和ParseNull同理,只是把null变为了true
                template<unsigned parseFlags, typename InputStream, typename Handler>
                void ParseTrue(InputStream& is, Handler& handler){
                    RAPIDJSON_ASSERT(is.Peek()=='t');// true第一个为't'
                    is.Take();
                    if(RAPIDJSON_LIKELY(Consume(is, 'r')&&Consume(is, 'u')&&Consume(is, 'e'))){
                        if(RAPIDJSON_UNLIKELY(!handler.Bool(true)))
                            RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                    }
                    else
                        RAPIDJSON_PARSE_ERROR(kParseErrorValueInvalid, is.Tell());
                }
                // 和ParseTrue同理,只是把true变为了false
                template<unsigned parseFlags, typename InputStream, typename Handler>
                void ParseFalse(InputStream& is, Handler& handler){
                    RAPIDJSON_ASSERT(is.Peek()=='f');// false第一个为'f'
                    is.Take();
                    if(RAPIDJSON_LIKELY(Consume(is, 'a')&&Consume(is, 'l')&&Consume(is, 's')&&Consume(is, 'e'))){
                        if(RAPIDJSON_UNLIKELY(!handler.Bool(false)))
                            RAPIDJSON_PARSE_ERROR(kParseErrorTermination, is.Tell());
                    }
                    else
                        RAPIDJSON_PARSE_ERROR(kParseErrorValueInvalid, is.Tell());
                }
                /**
                 * @brief 用于从输入流InputStream中读取一个字符,并检查是否于期望字符相等.如果相等,就把它从输入流中移除,返回true;不匹配就返回false
                 * 
                 * @tparam InputStream 
                 * @param is 
                 * @param expect 
                 * @return RAPIDJSON_FORCEINLINE 
                 */
                template<typename InputStream>
                RAPIDJSON_FORCEINLINE static bool Consume(InputStream& is, typename InputStream::Ch expect){
                    if(RAPIDJSON_LIKELY(is.Peek()==expect)){
                        is.Take();
                        return true;
                    }
                    else
                        return false;
                }
                /**
                 * @brief 用于解析一个4位16进制的Unicode转义序列(如\uXXXX的XXXX).
                 * 它从输入流中读取4个16进制字符,并将它们转换位一个unsigned类型的Unicode代码点
                 * @tparam InputStream 
                 * @param is 
                 * @param escapeOffset 
                 * @return unsigned 
                 */
                template<typename InputStream>
                unsigned ParseHex4(InputStream& is, size_t escapeOffset){
                    unsigned codepoint = 0;// 用于存储4个16进制字符解析后的结果
                    for(int i=0;i<4;i++){// 循环4次,解析4个16进制字符
                        Ch c = is.Peek();// 查看输入流中的当前字符,不移除它
                        codepoint <<= 4;// 将codepoint左移4位,为下一个16进制位腾出空间  每个16进制位解析后会占据4位,所以要左移4位
                        codepoint += static_cast<unsigned>(c);// 将当前字符转为数字,并加到codepoint中
                        if(c>='0'&&c<='9')// 数字字符
                            codepoint -= '0';// 将字符转换为数字
                        else if(c>='A'&&c<='F')// 大写字母A-F
                            codepoint -= 'A'-10;// 将字符转换位10到15的数字
                        else if(c>='a'&&c<='f')// 小写字母a-f
                            codepoint -= 'a'-10;// 将字符转换位10-16的数字
                        else{// 非法的16进制字符
                            RAPIDJSON_PARSE_ERROR_NORETURN(kParseErrorStringUnicodeEscapeInvalidHex, escapeOffset);
                            RAPIDJSON_PARSE_ERROR_EARLY_RETURN(0);
                        }
                        is.Take();
                    }
                    return codepoint;// 返回解析得到的Unicode代码点
                }
                /**
                 * @brief 提供一种基于栈的临时存储机制,用于存储字符或其它数据,并在需要时可以弹出这些数据
                 * 通常用于JSON解析器在非原地解析时构建字符串、数字或其它值时临时存放数据
                 * @tparam CharType 
                 */
                template<typename CharType>
                class StackStream{
                    public:
                        typedef CharType Ch;
                        StackStream(internal::Stack<StackAllocator>& stack) 
                            : stack_(stack),
                              length_(0)// 记录当前在栈中存放的字符数量
                            {}
                        RAPIDJSON_FORCEINLINE void Put(Ch c){
                            *stack_.template Push<Ch>() = c;
                            ++length_;
                        }
                        RAPIDJSON_FORCEINLINE void* Push(SizeType count){
                            length_ += count;
                            return stack_.template Push<Ch>(count);
                        }
                        size_t Length() const {return length_;}
                        Ch* Pop(){
                            return stack_.template Pop<Ch>(length_);// 弹出length_,即将栈恢复到调用Put和Push之前的状态
                        }
                    private:
                        StackStream(const StackStream&) = delete;
                        StackStream& operator=(const StackStream&) = delete;
                        internal::Stack<StackAllocator>& stack_;
                        SizeType length_;
                };
                /**
                 * @brief 解析JSON字符串值(键或值)
                 * 可以就地解析(直接修改原始输入流)也可以非就地解析(使用栈临时存储数据)
                 * @tparam parseFlags 
                 * @tparam InputStream 
                 * @tparam Handler 
                 * @param is 
                 * @param handler 
                 * @param isKey 
                 */
                template<unsigned parseFlags, typename InputStream, typename Handler>
                void ParseString(InputStream& is, Handler& handler, bool isKey=false){
                    internal::StreamLocalCopy<InputStream> copy(is);
                    InputStream& s(copy.s);
                    RAPIDJSON_ASSERT(s.Peek()=='\"');
                    s.Take();// 跳过'\"'
                    bool success = false;
                    if(parseFlags & kParseInsituFlag){// 就地解析  不会用栈来存储临时数据
                        typename InputStream::Ch *head = s.PutBegin();// 获取就地解析的起始位置
                        ParseStringToStream<parseFlags, SourceEncoding, SourceEncoding>(s, s);// 从输入流中逐个读取字符进行解析  就地解析的源编码和目标编码一致
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        size_t length = s.PutEnd(head)-1;// 获取字符串结束位置
                        RAPIDJSON_ASSERT(length <= 0xFFFFFFFF);
                        const typename TargetEncoding::Ch* const str = reinterpret_cast<typename TargetEncoding::Ch*>(head);
                        success = (isKey ? handler.Key(str, SizeType(length), false):handler.String(str, SizeType(length), false));// 如果是键就回调.Key();否则回调.String()
                    }
                    else{// 非就地解析  需要用栈->StackStream
                        StackStream<typename TargetEncoding::Ch> stackStream(stack_);
                        ParseStringToStream<parseFlags, SourceEncoding, TargetEncoding>(s, stackStream);
                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        SizeType length = static_cast<SizeType>(stackStream.Length())-1;// FUCK!!!  忘记-1,调试一下午
                        const typename TargetEncoding::Ch* const str = stackStream.Pop();
                        success = (isKey ? handler.Key(str, SizeType(length), true):handler.String(str, SizeType(length), true));// 用栈空间了,所以handler接口函数的copy=true
                    }
                    if(RAPIDJSON_UNLIKELY(!success))
                        RAPIDJSON_PARSE_ERROR(kParseErrorTermination, s.Tell());
                }
                /**
                 * @brief 用于处理JSON字符串的函数,它会解析输入流中的JSON字符串,处理转义字符,并将解析后的字符写入输出流
                 * ParseString()是ParseStringToStream()的上层封装
                 * @tparam parseFlags 
                 * @tparam SourceEncoding 
                 * @tparam TargetEncoding 
                 * @tparam OutputStream 
                 * @param is 
                 * @param os 
                 * @return RAPIDJSON_FORCEINLINE 
                 */
                template<unsigned parseFlags, typename SEncoding, typename TEncoding, typename InputStream, typename OutputStream>
                RAPIDJSON_FORCEINLINE void ParseStringToStream(InputStream& is, OutputStream& os){
                    //!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
                    #define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
                            static const char escape[256] = {
                                Z16, Z16, 0, 0,'\"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '/',
                                Z16, Z16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0,
                                0, 0,'\b', 0, 0, 0,'\f', 0, 0, 0, 0, 0, 0, 0,'\n', 0,
                                0, 0,'\r', 0,'\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16
                            };
                    #undef Z16
                    //!@endcond
                    while(true){
                        if(!(parseFlags&kParseValidateEncodingFlag))// 如果没有启用字符编码验证(即不会考虑转义字符这些),则快速复制非转义字符
                            ScanCopyUnescapedString(is, os);
                        Ch c = is.Peek();
                        // 处理转义字符  转义字符前肯定有个\\  这从Writer::WriteString()中的PutUnsafe(*os_, '\\');也可看出
                        if(RAPIDJSON_UNLIKELY(c=='\\')){
                            size_t escapeOffset = is.Tell();
                            is.Take();
                            Ch e = is.Peek();
                            // 检查并处理简单转义字符(即不包括Unicode转义序列和小于0x20的控制字符)
                            if(((sizeof(Ch)==1)||unsigned(e)<256)&&RAPIDJSON_LIKELY(escape[static_cast<unsigned char>(e)])){// 非转义字符且支持ASCII码
                                is.Take();
                                os.Put(static_cast<typename TEncoding::Ch>(escape[static_cast<unsigned char>(e)]));
                            }
                            // 检查是否启用解析单引号的转义字符
                            else if((parseFlags&kParseEscapedApostropheFlag)&&RAPIDJSON_LIKELY(e=='\'')){
                                is.Take();
                                os.Put('\'');
                            }
                            // 处理Unicode转义序列
                            else if(RAPIDJSON_LIKELY(e=='u')){
                                is.Take();
                                unsigned codepoint = ParseHex4(is, escapeOffset); // 解析4位十六进制数字为Unicode码点
                                RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                                // 检查是否为UTF16的代理对  UTF16中:高代理:0xD800-0xDBFF;低代理:0xDC00-0xDFFF
                                if(RAPIDJSON_UNLIKELY(codepoint>=0xD800&&codepoint<=0xDFFF)){// 代理对范围
                                    if(RAPIDJSON_LIKELY(codepoint<=0xD8FF)){// 高代理
                                        if(RAPIDJSON_UNLIKELY(!Consume(is, '\\')||!Consume(is, 'u')))// 确保是以\u开始
                                            RAPIDJSON_PARSE_ERROR(kParseErrorStringUnicodeSurrogateInvalid, escapeOffset);
                                        unsigned codepoint2 = ParseHex4(is, escapeOffset);// 解析第二个\uxxx序列以获得低代理
                                        RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID;
                                        if(RAPIDJSON_UNLIKELY(codepoint2<0xDC00||codepoint2>0xDFFF))// 检查codepoint2是否处于低代理范围
                                            RAPIDJSON_PARSE_ERROR(kParseErrorStringUnicodeSurrogateInvalid, escapeOffset);
                                        codepoint = (((codepoint-0xD800)<<10)|(codepoint2-0xDC00))+0x10000;// 合并低代理和高代理
                                    }
                                    else// 只有低代理而没有匹配的高代理,报错   在Unicode转义序列中高代理在低代理前面,所以不可能只有单独的低代理
                                        RAPIDJSON_PARSE_ERROR(kParseErrorStringUnicodeSurrogateInvalid, escapeOffset);
                                }
                                TEncoding::Encode(os, codepoint);// codepoint->os中
                            }
                            else
                                RAPIDJSON_PARSE_ERROR(kParseErrorStringEscapeInvalid, escapeOffset);    
                        }
                        else if(RAPIDJSON_UNLIKELY(c=='"')){// 检查结束双引号
                            is.Take();
                            os.Put('\0');
                            return;
                        }
                        else if(RAPIDJSON_UNLIKELY(static_cast<unsigned>(c)<0x20)){// 检查控制字符(非法字符)
                            if(c=='\0')
                                RAPIDJSON_PARSE_ERROR(kParseErrorStringMissQuotationMark, is.Tell());
                            else
                                RAPIDJSON_PARSE_ERROR(kParseErrorStringInvalidEncoding, is.Tell());
                        }
                        else{// 非转义字符或普通字符
                            size_t offset = is.Tell();
                            if (RAPIDJSON_UNLIKELY((parseFlags & kParseValidateEncodingFlag ?
                                !Transcoder<SEncoding, TEncoding>::Validate(is, os) :
                                !Transcoder<SEncoding, TEncoding>::Transcode(is, os))))
                                RAPIDJSON_PARSE_ERROR(kParseErrorStringInvalidEncoding, offset);
                        }
                    }
                }  
                // 什么都不做的ScanCopyUnescapedString()函数
                template<typename InputStream, typename OutputStream>
                static RAPIDJSON_FORCEINLINE void ScanCopyUnescapedString(InputStream&, OutputStream&) {}
                #if defined(RAPIDJSON_SSE2) || defined(RAPIDJSON_SSE42)
                    /**
                     * @brief 批量读取输入流中的字符,检测是否包含特殊字符(双引号、反斜杠、控制字符),并将无特殊字符的部分直接写入输出流.遇到特殊字符直接返回
                     * 这个函数其实和Writer::ScanWriteUnescapedString()基本一模一样
                     * @param is 
                     * @param os 
                     * @return RAPIDJSON_FORCEINLINE 
                     */
                    static RAPIDJSON_FORCEINLINE void ScanCopyUnescapedString(StringStream& is, StackStream<char>& os){
                        // StringStream -> StackStream<char>
                        const char* p = is.src_;
                        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址    向上对齐
                        while (p != nextAligned)// 未对齐部分逐字符单独处理   不用SSE处理
                            if (RAPIDJSON_UNLIKELY(*p == '\"') || RAPIDJSON_UNLIKELY(*p == '\\') || RAPIDJSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {// 如果是控制字符、双引号或反斜杠=>特殊字符
                                is.src_ = p;
                                return;
                            }
                            else
                                os.Put(*p++);// 不是特殊字符,则直接写入输出流
                        // 定义SIMD常量
                        static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };// 双引号对应的SSE常量
                        static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };// 反斜杠对应的SSE常量
                        static const char space[16]  = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };// 控制字符上限  即小于0x1F的都是控制字符
                        const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&dquote[0]));// 加载dquote这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
                        const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
                        const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));
                        // 对16字节对齐的部分进行SSE批量处理 即一次处理16字节
                        for (;; p += 16) {// 每次读取16个字节的数据块
                            const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));// 从p加载16字节数据到寄存器s中
                            const __m128i t1 = _mm_cmpeq_epi8(s, dq);// 逐字节比较比较是否为双引号
                            const __m128i t2 = _mm_cmpeq_epi8(s, bs);// 逐字节比较比较是否为反斜杠
                            const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp);// 逐字节比较比较是否为控制字符
                            const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);// 合并比较结果
                            unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8(x));// 生成掩码  如果掩码r中某位=1,则表示对应的字符是特殊字符
                            if (RAPIDJSON_UNLIKELY(r != 0)) {// r不为0,则表示合并的结果中出现了特殊字符
                                SizeType len;
                                len = static_cast<SizeType>(__builtin_ffs(r) - 1);// 查找第一个非零位 则非零位前都是无转义字符
                                char* q = reinterpret_cast<char*>(os.Push(len));
                                for(size_t i=0;i<len;++i)
                                    q[i] = p[i];// 拷贝无转义字符的部分 len之前的是普通字符
                                p += len;// 更新p位置以跳过已处理的普通字符部分
                                break;// 当扫描到特殊字符,就停止扫描,即退出函数
                            }
                            _mm_storeu_si128(reinterpret_cast<__m128i*>(os.Push(16), s);// 无特殊字符,则直接写入输出流
                        }
                        is.src_ = p;
                    }
                    /**
                     * @brief 用于在源和目标流相同的情况下(即原地解析)复制字符串内容(原地处理也可能需要拷贝,即此时的is.src_可能和is.dst_可能不相同)
                     * 
                     * @param is 
                     * @param os 
                     * @return RAPIDJSON_FORCEINLINE 
                     */
                    static RAPIDJSON_FORCEINLINE void ScanCopyUnescapedString(InsituStringStream& is, InsituStringStream& os){
                        // InsituStringStream -> InsituStringStream
                        RAPIDJSON_ASSERT(&is==&os);// 确保输入流和输出流是同一个流对象
                        (void)os;
                        if(is.src_==is.dst_){// 如果输入流的读取数据位置和目标写入位置一样,则此时无需拷贝,直接SkipUnescapedString进行跳过未转义的字符串就行
                            SkipUnescapedString(is);
                            return;
                        }
                        // 源流的读取位置和写入位置不一致,则需要拷贝操作(就算是输入流对象和输出流对象一致也需要,因为这是对于流对象内的内存位置不一致)
                        char* p = is.src_;// 指向源字符串的起始位置,即读取位置
                        char *q = is.dst_;// 指向目标位置,即写入位置
                        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址    向上对齐
                        while (p != nextAligned)// 未对齐部分逐字符单独处理   不用SSE处理
                            if (RAPIDJSON_UNLIKELY(*p == '\"') || RAPIDJSON_UNLIKELY(*p == '\\') || RAPIDJSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {// 如果是控制字符、双引号或反斜杠=>特殊字符
                                is.src_ = p;
                                is.dst_ = q;
                                return;
                            }
                            else
                                *q++ = *p++;// 不是特殊字符,则直接写入输出流
                        // 定义SIMD常量
                        static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };// 双引号对应的SSE常量
                        static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };// 反斜杠对应的SSE常量
                        static const char space[16]  = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };// 控制字符上限  即小于0x1F的都是控制字符
                        const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&dquote[0]));// 加载dquote这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
                        const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
                        const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));
                        // 对16字节对齐的部分进行SSE批量处理 即一次处理16字节
                        for (;; p+=16,q+=16) {// 每次读取16个字节的数据块
                            const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));// 从p加载16字节数据到寄存器s中
                            const __m128i t1 = _mm_cmpeq_epi8(s, dq);// 逐字节比较比较是否为双引号
                            const __m128i t2 = _mm_cmpeq_epi8(s, bs);// 逐字节比较比较是否为反斜杠
                            const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp);// 逐字节比较比较是否为控制字符
                            const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);// 合并比较结果
                            unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8(x));// 生成掩码  如果掩码r中某位=1,则表示对应的字符是特殊字符
                            if (RAPIDJSON_UNLIKELY(r != 0)) {// r不为0,则表示合并的结果中出现了特殊字符
                                SizeType len;
                                len = static_cast<SizeType>(__builtin_ffs(r) - 1);// 查找第一个非零位 则非零位前都是无转义字符
                                for(const char* pend=p+len;p!=pend;)// 从源指针p到目标指针q的逐字节拷贝,直到p达到pend
                                    *q++ = *p++;// 逐字节拷贝
                                break;// 当扫描到特殊字符,就停止扫描,即退出函数
                            }
                            _mm_storeu_si128(reinterpret_cast<__m128i*>(q, s);// 无特殊字符,则直接复制到q
                        }
                        is.src_ = p;
                        is.dst_ = q;
                    }
                    /**
                     * @brief 跳过字符串中的非转义字符并寻找特殊字符.当找到特殊字符时,就停止扫描并将流的当前指针更新为该位置
                     * 
                     * @param is 
                     * @return RAPIDJSON_FORCEINLINE 
                     */
                    static RAPIDJSON_FORCEINLINE void SkipUnescapedString(InsituStringStream& is) {
                        RAPIDJSON_ASSERT(is.src_==is.dst_);// 确保流is的读取位置和写入位置一致
                        char* p = is.src_;
                        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
                        while(p != nextAligned)
                            if (RAPIDJSON_UNLIKELY(*p == '\"') || RAPIDJSON_UNLIKELY(*p == '\\') || RAPIDJSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                                is.src_ = is.dst_ = p;
                                return;
                            else
                                p++;
                            }
                        // 定义SIMD常量
                        static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };// 双引号对应的SSE常量
                        static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };// 反斜杠对应的SSE常量
                        static const char space[16]  = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };// 控制字符上限  即小于0x1F的都是控制字符
                        const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&dquote[0]));// 加载dquote这个16字节常量到SIMD寄存器中,以便于后续用于批量比较
                        const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
                        const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));
                        // 对16字节对齐的部分进行SSE批量处理 即一次处理16字节
                        for (;; p += 16) {// 每次读取16个字节的数据块
                            const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));// 从p加载16字节数据到寄存器s中
                            const __m128i t1 = _mm_cmpeq_epi8(s, dq);// 逐字节比较比较是否为双引号
                            const __m128i t2 = _mm_cmpeq_epi8(s, bs);// 逐字节比较比较是否为反斜杠
                            const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp);// 逐字节比较比较是否为控制字符
                            const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);// 合并比较结果
                            unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8(x));// 生成掩码  如果掩码r中某位=1,则表示对应的字符是特殊字符
                            if (RAPIDJSON_UNLIKELY(r != 0)) {// r不为0,则表示合并的结果中出现了特殊字符
                                SizeType len;
                                len = static_cast<SizeType>(__builtin_ffs(r) - 1);// 查找第一个非零位 则非零位前都是无转义字符
                                p += len;// 更新p位置以跳过已处理的普通字符部分
                                break;// 当扫描到特殊字符,就停止扫描,即退出函数
                            }
                            is.src_ = is.dst_ = p;
                        }
                    }
                    #elif defined(RAPIDJSON_NEON)
                        /**
                         * @brief 利用NEON指令集批量读取输入流中的字符,检测是否包含特殊字符(双引号、反斜杠、控制字符),并将无特殊字符的部分直接写入输出流.遇到特殊字符直接返回
                         * 
                         * @param is 
                         * @param os 
                         * @return RAPIDJSON_FORCEINLINE 
                         */
                        static RAPIDJSON_FORCEINLINE void ScanCopyUnescapedString(StringStream& is, StackStream<char>& os){
                            // StringStream -> StackStream<char>
                            const char* p = is.src_;
                            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址    向上对齐
                            while (p != nextAligned)// 未对齐部分逐字符单独处理   不用SSE处理
                                if (RAPIDJSON_UNLIKELY(*p == '\"') || RAPIDJSON_UNLIKELY(*p == '\\') || RAPIDJSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {// 如果是控制字符、双引号或反斜杠=>特殊字符
                                    is.src_ = p;
                                    return;
                                }
                                else
                                    os.Put(*p++);// 不是特殊字符,则直接写入输出流
                            // 定义NEON常量
                            const uint8x16_t s0 = vmovq_n_u8('"');// 使用vmovq_n_u8加载双引号到NEON寄存器
                            const uint8x16_t s1 = vmovq_n_u8('\\');// 使用vmovq_n_u8加载反斜杠到NEON寄存器
                            const uint8x16_t s2 = vmovq_n_u8('\b');// 使用vmovq_n_u8加载退格符到NEON寄存器   NEON指令集中对退格符\b单独进行检测了,而不是纳入控制字符中被统一检测
                            const uint8x16_t s3 = vmovq_n_u8(32);// 使用vmovq_n_u8加载控制字符到NEON寄存器  32<=>0x20
                            // 对16字节对齐的部分进行SSE批量处理 即一次处理16字节
                            for (;; p += 16) {// 每次读取16个字节的数据块
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
                                    char* q = reinterpret_cast<char*>(os.Push(len));
                                    for(size_t i=0;i<len;++i)
                                        q[i] = p[i];
                                    p += len;
                                    break;
                                }
                                vst1q_u8(reinterpret_cast<uint8_t *>(os.Push(16)), s);// 这个16字节块无特殊字符 则直接写入
                            }
                            is.src_ = p;
                        }
                        /**
                         * @brief 利用NEON指令集,用于在源和目标流相同的情况下(即原地解析)复制字符串内容(原地处理也可能需要拷贝,即此时的is.src_可能和is.dst_可能不相同)
                         * 
                         * @param is 
                         * @param os 
                         * @return RAPIDJSON_FORCEINLINE 
                         */
                        static RAPIDJSON_FORCEINLINE void ScanCopyUnescapedString(InsituStringStream& is, InsituStringStream& os){
                            // InsituStringStream -> InsituStringStream
                            RAPIDJSON_ASSERT(&is==&os);// 确保输入流和输出流是同一个流对象
                            (void)os;
                            if(is.src_==is.dst_){// 如果输入流的读取数据位置和目标写入位置一样,则此时无需拷贝,直接SkipUnescapedString进行跳过未转义的字符串就行
                                SkipUnescapedString(is);
                                return;
                            }
                            // 源流的读取位置和写入位置不一致,则需要拷贝操作(就算是输入流对象和输出流对象一致也需要,因为这是对于流对象内的内存位置不一致)
                            char* p = is.src_;// 指向源字符串的起始位置,即读取位置
                            char *q = is.dst_;// 指向目标位置,即写入位置
                            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));// 计算下一个16字节对齐的地址    向上对齐
                            while (p != nextAligned)// 未对齐部分逐字符单独处理   不用SSE处理
                                if (RAPIDJSON_UNLIKELY(*p == '\"') || RAPIDJSON_UNLIKELY(*p == '\\') || RAPIDJSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {// 如果是控制字符、双引号或反斜杠=>特殊字符
                                    is.src_ = p;
                                    is.dst_ = q;
                                    return;
                                }
                                else
                                    *q++ = *p++;// 不是特殊字符,则直接写入输出流
                            // 定义NEON常量
                            const uint8x16_t s0 = vmovq_n_u8('"');// 使用vmovq_n_u8加载双引号到NEON寄存器
                            const uint8x16_t s1 = vmovq_n_u8('\\');// 使用vmovq_n_u8加载反斜杠到NEON寄存器
                            const uint8x16_t s2 = vmovq_n_u8('\b');// 使用vmovq_n_u8加载退格符到NEON寄存器   NEON指令集中对退格符\b单独进行检测了,而不是纳入控制字符中被统一检测
                            const uint8x16_t s3 = vmovq_n_u8(32);// 使用vmovq_n_u8加载控制字符到NEON寄存器  32<=>0x20
                            // 对16字节对齐的部分进行SSE批量处理 即一次处理16字节
                            for (;; p += 16,q += 16) {// 每次读取16个字节的数据块
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
                                        escaped = true;// 检测到特殊字符
                                    }
                                }
                                else{// low非零,则通过clzll计算零位数,即算无特殊字符部分的长度
                                    uint32_t lz = internal::clzll(low);
                                    len = lz>>3;
                                    escaped = true;// 检测到特殊字符
                                }
                                if(RAPIDJSON_UNLIKELY(escaped)){// 写入无特殊字符的部分并跳出
                                    for(const char* pend=p+len;p!=pend;)
                                        *q++ = *p++;
                                    break;
                                }
                                vst1q_u8(reinterpret_cast<uint8_t *>(q, s);// 这个16字节块无特殊字符 则直接写入
                            }
                            is.src_ = p;
                            is.dst_ = q;
                        } 
                    /**
                     * @brief 利用NEON指令集跳过字符串中的非转义字符并寻找特殊字符.当找到特殊字符时,就停止扫描并将流的当前指针更新为该位置
                     * 
                     * @param is 
                     * @return RAPIDJSON_FORCEINLINE 
                     */
                    static RAPIDJSON_FORCEINLINE void SkipUnescapedString(InsituStringStream& is) {
                        RAPIDJSON_ASSERT(is.src_==is.dst_);// 确保流is的读取位置和写入位置一致
                        char* p = is.src_;
                        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
                        while(p != nextAligned)
                            if (RAPIDJSON_UNLIKELY(*p == '\"') || RAPIDJSON_UNLIKELY(*p == '\\') || RAPIDJSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                                is.src_ = is.dst_ = p;
                                return;
                            else
                                p++;
                            }
                        // 定义NEON常量
                        const uint8x16_t s0 = vmovq_n_u8('"');// 使用vmovq_n_u8加载双引号到NEON寄存器
                        const uint8x16_t s1 = vmovq_n_u8('\\');// 使用vmovq_n_u8加载反斜杠到NEON寄存器
                        const uint8x16_t s2 = vmovq_n_u8('\b');// 使用vmovq_n_u8加载退格符到NEON寄存器   NEON指令集中对退格符\b单独进行检测了,而不是纳入控制字符中被统一检测
                        const uint8x16_t s3 = vmovq_n_u8(32);// 使用vmovq_n_u8加载控制字符到NEON寄存器  32<=>0x20
                        // 对16字节对齐的部分进行SSE批量处理 即一次处理16字节
                        for (;; p += 16) {// 每次读取16个字节的数据块
                            const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t*>(p));
                            uint8x16_t x = vceqq_u8(s, s0);// 使用vceqq_u8比较字符是否为特殊字符s0
                            x = vorrq_u8(x, vceqq_u8(s, s1));// 使用vceqq_u8比较字符是否为特殊字符s1
                            x = vorrq_u8(x, vceqq_u8(s, s2));// 使用vceqq_u8比较字符是否为特殊字符s2
                            x = vorrq_u8(x, vcltq_u8(s, s3));// 使用vceqq_u8比较字符是否小于控制字符界限s3(小于控制字符界限就说明它是控制字符)
                            x = vrev64q_u8(x);// 对x进行64位反转(前8字节和后8字节交换)
                            uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);// 反转后的64位低位,代表检测结果的掩码
                            uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);// 反转后的64位高位,代表检测结果的掩码
                            // 计算16字节块中特殊字符的索引,即算无特殊字符部分的长度len
                            // SSE指令集用__builtin_ffs()就能完成下面if-else中len1的计算
                            if(low==0){// low为全零,表示低位无特殊字符
                                if(high!=0){// high非零,则通过clzll计算零位数,即算无特殊字符部分的长度
                                    uint32_t lz = internal::clzll(high);
                                    len = 8+(lz>>3);
                                    break;// 因为is.src_=is.dst_,所以对于检测到了特殊字符,也不需要去处理非特殊字符的拷贝,因为此时不会涉及拷贝操作,因此直接在这break
                                }
                            }
                            else{// low非零,则通过clzll计算零位数,即算无特殊字符部分的长度
                                uint32_t lz = internal::clzll(low);
                                len = lz>>3;
                                break;
                            }
                        }
                        is.src_ = is.dst_ = p;
                    }
                    #endif// RAPIDJSON_NEON
                    /**
                     * @brief 用于高效地解析数值字符串
                     * 
                     * @tparam InputStream 
                     * @tparam StackCharacter:栈字符类型
                     * @tparam backup:是否需要备份  为true表示解析的数值字符串需要被保存到一个栈中,false则不备份解析后的数据
                     * @tparam pushOnTake:是否在获取字符时同时将字符推入栈   为true表示解析获取到的数字字符就立刻将字符存入栈,false则不立刻存入
                     */
                    template<typename InputStream, typename StackCharacter, bool backup, bool pushOnTake>
                    class NumberStream;// 通用版本
                    /**
                     * @brief 特化版本   不备份数据,不推入栈,适用于无需存储解析数据的情况
                     * 
                     * @tparam InputStream 
                     * @tparam StackCharacter 
                     */
                    template<typename InputStream, typename StackCharacter>
                    class NumberStream<InputStream, StackCharacter, false, false>{
                        public:
                            typedef typename InputStream::Ch Ch;
                            NumberStream(GenericReader& reader, InputStream& s) : is(s) {(void)reader;}
                            RAPIDJSON_FORCEINLINE Ch Peek() const {return is.Peek();}
                            RAPIDJSON_FORCEINLINE Ch TakePush() {return is.Take();}
                            RAPIDJSON_FORCEINLINE Ch Take() {return is.Take();}
                            RAPIDJSON_FORCEINLINE void Push(char) {}
                            size_t Tell() {return is.Tell();}
                            size_t Length() {return 0;}
                            const StackCharacter* Pop() {return 0;}
                        protected:
                            NumberStream& operator=(const NumberStream&) = delete;
                            InputStream& is;
                    };
                    /**
                     * @brief 特化版本 备份数据,但不立刻在解析完数据就推入栈中,此时的推入是由手动控制
                     * 
                     * @tparam InputStream 
                     * @tparam StackCharacter 
                     */
                    template<typename InputStream, typename StackCharacter>
                    class NumberStream<InputStream, StackCharacter, true, false> : public NumberStream<InputStream, StackCharacter, false, false>{
                        typedef NumberStream<InputStream, StackCharacter, false, false> Base;
                        public:
                            NumberStream(GenericReader& reader, InputStream& s) 
                                : Base(reader, s),
                                  stackStream(reader.stack_)
                                {}
                            // 将当前字符推入栈并越过它
                            RAPIDJSON_FORCEINLINE Ch TakePush(){
                                stackStream.Put(static_cast<StackCharacter>(Base::is.Peek()));
                                return Base::is.Take();
                            }
                            // 手动推入字符c入栈
                            RAPIDJSON_FORCEINLINE void Push(StackCharacter c){
                                stackStream.Put(c);
                            }
                            size_t Length() { return stackStream.Length();}
                            const StackCharacter* Pop(){
                                stackStream.Put('\0');
                                return stackStream.Pop();
                            }
						private:
							StackStream<StackCharacter> stackStream;
                    };
                    /**
                     * @brief 特化版本 备份数据,且立刻在解析一个字符后就会把它推入栈中(注意:解析的动作在这不会体现,在ParseNumber()体现)
                     * 
                     * @tparam InputStream 
                     * @tparam StackCharacter 
                     */
                    template<typename InputStream, typename StackCharacter>
                    class NumberStream<InputStream, StackCharacter, true, true> : public NumberStream<InputStream, StackCharacter, true, false>{
                        typedef NumberStream<InputStream, StackCharacter, true, false> Base;
                        public:
                            NumberStream(GenericReader& reader, InputStream& s) 
                                : Base(reader, s)
                                {}
                            // 每次在解析时会调用Take(),而此特化版本确保了在每次解析一个数值字符调用Take()时就会立刻把它压入栈中
                            RAPIDJSON_FORCEINLINE Ch Take() {return Base::TakePush();}
                    };
                    /**
                     * @brief 解析输入流InputStream中的数值字符串到i,i64,d中,并根据Handler接口调用相应的回调函数
                     * 
                     * @tparam parseFlags 
                     * @tparam InputStream 
                     * @tparam Handler 
                     * @param is 
                     * @param handler 
                     */
                    template<unsigned parseFlags, typename InputStream, typename Handler>
                    void ParseNumber(InputStream& is, Handler& handler){
                        // 如果parseFlags包含kParseNumbersAsStringsFlag,则使用TargetEncoding::Ch
                        typedef typename internal::SelectIf<internal::BoolType<(parseFlags&kParseNumbersAsStringsFlag)!=0>, typename TargetEncoding::Ch, char>::Type NumberCharacter;
                        internal::StreamLocalCopy<InputStream> copy(is);// 创建InputStream的本地副本copy(如StringStream(其copyOptimization=1,因此会创建副本))
                        // 根据parseFlags确定是否解析成字符串、是否原地解析、是否使用全精度等的NumberStream对象
                        NumberStream<InputStream, NumberCharacter,
                            ((parseFlags&kParseNumbersAsStringsFlag)!=0) ?
                                ((parseFlags&kParseInsituFlag)==0) : 
                                ((parseFlags&kParseFullPrecisionFlag)!=0),
                            (parseFlags&kParseNumbersAsStringsFlag)!=0 &&
                                (parseFlags&kParseInsituFlag)==0> s(*this, copy.s);
                        size_t startOffset = s.Tell();// 处理数值字符串时的起始偏移量
                        double d = 0.0;
                        bool useNanOrInf = false;// 标记是否解析到NaN或无穷大
                        bool minus = Consume(s, '-');// 检查是否有负号'-'
                        unsigned i = 0;// 用于存储较小的整数
                        uint64_t i64 = 0;// 用于存储较大的整数
                        bool use64bit = false;// 表示是否需要使用64位整数
                        int significandDigit = 0;// 用来跟踪有效数字的位数
                        if(RAPIDJSON_UNLIKELY(s.Peek()=='0')){// 检查第一个字符是否为'0'.如果是,则解析为0并推进栈中
                            i = 0;
                            s.TakePush();
                        }
                        else if(RAPIDJSON_LIKELY(s.Peek()>='1'&&s.Peek()<='9')){// 如果第一个字符是1到9之间的数字,则解析该数字并存储到i中
                            i = static_cast<unsigned>(s.TakePush()-'0');
                            if(minus)
                                while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9')){
                                    // 32位负数的上限 2^31 = 2147483648
                                    if(RAPIDJSON_UNLIKELY(i>=214748364)){// 检查是否接近-2^31.判断214748364的原因是此时再读入一个大于等于8的字符将使数值超出范围
                                        if(RAPIDJSON_LIKELY(i!=214748364||s.Peek()>'8')){// 如果i已达到214748364,若再读入一个字符大于等于'8'的字符,就超出32位的上限,就要用64位来存储了
                                            i64 = i;
                                            use64bit = true;
                                            break;
                                        }
                                    }
                                    i = i*10+static_cast<unsigned>(s.TakePush()-'0');
                                    significandDigit++;
                                }
                            else
                                while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9')){
                                    // 32位正数的上限 2^32-1 = 4294967295
                                    if(RAPIDJSON_UNLIKELY(i>=429496729)){// 检查是否接近2^32-1.判断429496729的原因是此时再读入一个大于等于5的字符将使数值超出范围
                                        if(RAPIDJSON_LIKELY(i!=429496729||s.Peek()>'5')){// 如果i已达到429496729,若再读入一个字符大于等于'5'的字符,就超出32位的上限,就要用64位来存储了
                                            i64 = i;
                                            use64bit = true;
                                            break;
                                        }
                                    }
                                    i = i*10+static_cast<unsigned>(s.TakePush()-'0');// 没有超过32位的存储上限,就继续解析数值字符,然后将其累积到32位的i中
                                    significandDigit++;
                                }
                        }
                        // 处理NaN或Infinity
                        else if((parseFlags&kParseNanAndInfFlag)&&RAPIDJSON_LIKELY((s.Peek()=='I'||s.Peek()=='N'))){
                            if(Consume(s, 'N')){
                                if(Consume(s, 'a')&&Consume(s, 'N')){
                                    d = std::numeric_limits<double>::quiet_NaN();
                                    useNanOrInf = true;
                                }
                            }
                            else if(RAPIDJSON_LIKELY(Consume(s, 'I'))){
                                if(Consume(s, 'n')&&Consume(s, 'f')){
                                    d = (minus ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity());// 负无穷和正无穷
                                    useNanOrInf = true;
                                    if (RAPIDJSON_UNLIKELY(s.Peek() == 'i' && !(Consume(s, 'i') && Consume(s, 'n')
                                        && Consume(s, 'i') && Consume(s, 't') && Consume(s, 'y')))){
                                            RAPIDJSON_PARSE_ERROR(kParseErrorValueInvalid, s.Tell());
                                    }
                                }
                            }
                            if(RAPIDJSON_UNLIKELY(!useNanOrInf))
                                RAPIDJSON_PARSE_ERROR(kParseErrorValueInvalid, s.Tell());
                        }
                        else
                            RAPIDJSON_PARSE_ERROR(kParseErrorValueInvalid, s.Tell());
                        bool useDouble = false;// 标志是否将数值转换为double类型(如果超出64位范围就会用double)
                        if(use64bit){// 使用64位的i64来继续存储后续解析的数值字符
                            if(minus)
                                while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9')){
                                    // 64位负数的上限  2^63 = 9223372036854775808
                                    if(RAPIDJSON_UNLIKELY(i64>=RAPIDJSON_UINT64_C2(0x0CCCCCCC, 0xCCCCCCCC))){// 检查是否接近-2^63.判断922337203685477580的原因是此时再读入一个大于等于8的字符将使数值超出范围
                                        if(RAPIDJSON_LIKELY(i64!=RAPIDJSON_UINT64_C2(0x0CCCCCCC, 0xCCCCCCCC)||s.Peek()>'8')){// 如果i已达到922337203685477580,若再读入一个字符大于等于'8'的字符,就超出64位的上限,就要用double来存储了
                                            d = static_cast<double>(i64);
                                            useDouble = true;// 超出64位就用double
                                            break;
                                        }
                                    }
                                    i64 = i64*10+static_cast<unsigned>(s.TakePush()-'0');// 没有超过64位的存储上限,就继续解析数值字符,然后将其累积到64位的i64中
                                    significandDigit++;
                                }
                            else
                                while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9')){
                                    // 32位正数的上限 2^64-1 = 18446744073709551615
                                    if(RAPIDJSON_UNLIKELY(i64>=RAPIDJSON_UINT64_C2(0x19999999, 0x99999999))){// 检查是否接近2^64-1.判断1844674407370955161的原因是此时再读入一个大于等于5的字符将使数值超出范围
                                        if(RAPIDJSON_LIKELY(i64!=RAPIDJSON_UINT64_C2(0x19999999, 0x99999999)||s.Peek()>'5')){// 如果i已达到1844674407370955161,若再读入一个字符大于等于'5'的字符,就超出64位的上限,就要用double来存储了
                                            d = static_cast<double>(i64);
                                            useDouble = true;// 超出64位就用double
                                            break;
                                        }
                                    }
                                    i64 = i64*10+static_cast<unsigned>(s.TakePush()-'0');// 没有超过64位的存储上限,就继续解析数值字符,然后将其累积到64位的i64中
                                    significandDigit++;
                                }  
                        }
                        if(useDouble){// 转换为double继续累积输入流的数值字符串的解析结果
                            while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9'))
                                d = d*10+(s.TakePush()-'0');
                        }
                        int expFrac = 0;// 记录小数点后的位数
                        size_t decimalPosition;// 记录小数点的位置,用于将小数部分转换为double
                        if(Consume(s, '.')){// 检查当前字符是否是小数点
                            decimalPosition = s.Length();
                            if(RAPIDJSON_UNLIKELY(!(s.Peek()>='0')&&s.Peek()<='9'))// 小数点后没有有效数字字符则报错
                                RAPIDJSON_PARSE_ERROR(kParseErrorNumberMissFraction, s.Tell());
                                // 这里的if中是在d中保存精确值,即double中最多53位
                                if(!useDouble){// 如果小数点前的整数部分没用double类型,就将其转换为double类型(i->i64->double,不是从32位的i直接转换为double)
                                    #if RAPIDJSON_64BIT
                                        if(!use64bit)// 没用使用64位的时候,就将原来的整数部分i赋值给i64,以便后面转换为double
                                            i64 = i;
                                        while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9')){
                                            if(i64>RAPIDJSON_UINT64_C2(0x1FFFFF, 0xFFFFFFFF))// RAPIDJSON_UINT64_C2(0x1FFFFF, 0xFFFFFFFF)表示2^53-1,double类型的尾数最大为53位,因此对于超过53位的数double就不能精确表示,因为这里的d保存的是精确值,因此当大于53位时就直接break
                                                break;
                                            else{// 当有效数字位数不达53位,那么可以直接把小数部分往之前的整数部分i64中累积
                                                i64 = i64*10+static_cast<unsigned>(s.TakePush()-'0');
                                                --expFrac;
                                                if(i64!=0)
                                                    significandDigit++;
                                            }
                                        }
                                        d = static_cast<double>(i64);
                                    #else
                                        d = static_cast<double>(use64bit ? i64 : i);
                                    #endif
                                        useDouble = true;
                                }
                                // 前面d保存的是精确的,不会大于53位   如果前面不大于53位,那么s.Peek()就不会是数值字符
                                // 如果前面的整数部分直接用double存储了,此时就也是类似处理  十进制有效位数不超过 17 位的情况(等价于二进制不超过53位)
                                while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9')){
                                    if(significandDigit < 17){// double类型一般十进制精度最大为17
                                        d = d*10.0+(s.TakePush()-'0');
                                        --expFrac;
                                        if(RAPIDJSON_LIKELY(d > 0.0))
                                            significandDigit++;
                                    }
                                    else// 如果小数点后的十进制位数超过17位,则使用TakePush()消耗字符但不存储,以忽略超出部分
                                        s.TakePush();
                                }
                        }
                        else
                            decimalPosition = s.Length();
                        int exp = 0;// 用于存储解析到的指数值
                        if(Consume(s, 'e') || Consume(s, 'E')){// 如果找到e||E,则进入指数部分的逻辑
                            if(!useDouble){// 如果没有使用double来存储数值,则将整数部分先转换为double,便于直接将后续的指数部分转换为double,并累积上去
                                d = static_cast<double>(use64bit ? i64:i);
                                useDouble = true;
                            }
                            bool expMinus = false;// 用于记录指数的符号是否为负号
                            if(Consume(s, '+'))// 正数指数
                                ;
                            else if(Consume(s, '-'))// 负数指数
                                expMinus = true;
                            if(RAPIDJSON_LIKELY(s.Peek() >= '0' && s.Peek() <= '9')){
                                exp = static_cast<int>(s.Take()-'0');// 将指数的第一个数字字符转换为整数并存储到exp中
                                if(expMinus){// 检查为负指数
                                    RAPIDJSON_ASSERT(expFrac <= 0);
                                    int maxExp = (expFrac + 2147483639)/10;// 负数时对应的负指数最大值,以防止溢出   maxExp用于防止exp的值变得过大(避免整数下溢),以确保-exp不会超出int的范围
                                    while(RAPIDJSON_LIKELY(s.Peek() >= '0' && s.Peek() <= '9')){
                                        exp = exp*10+static_cast<int>(s.Take()-'0');
                                        if(RAPIDJSON_UNLIKELY(exp>maxExp)){// 如果exp超过maxExp的值,则停止解析,防止exp变得过大
                                            while(RAPIDJSON_LIKELY(s.Peek() >= '0' && s.Peek() <= '9'))
                                                s.Take();
                                        }
                                    }
                                }
                                else{// 检查为正指数
                                    int maxExp = 308-expFrac;// 正数时对应的正指数最大值,以防指数溢出
                                    while(RAPIDJSON_LIKELY(s.Peek() >= '0' && s.Peek() <= '9')){
                                        exp = exp*10+static_cast<int>(s.Take()-'0');
                                        if (RAPIDJSON_UNLIKELY(exp > maxExp))// 如果exp超过maxExp的值,则停止解析,防止exp变得过大
                                            RAPIDJSON_PARSE_ERROR(kParseErrorNumberTooBig, startOffset);
                                        }
                                    }
                                }
                                else// 如果在'e'或'E'后没有数字字符,则报错
                                    RAPIDJSON_PARSE_ERROR(kParseErrorNumberMissExponent, s.Tell());
                                if(expMinus)
                                    exp = -exp;
                            }
                            bool cont = true;// 用于标识解析是否成功
                            if(parseFlags&kParseNumbersAsStringsFlag){// 将原始字符串中的数字作为字符串处理,而不是转换为数值再后续处理
                                if(parseFlags&kParseInsituFlag){// 在原始输入流上直接进行操作,而不是复制数据再进行后续操作
                                    s.Pop();// 清理栈,因为此时是原地处理,所以只会涉及输入流is,而为了将栈的状态恢复到解析数字之前的状态,避免数据残留,就要Pop()
                                    typename InputStream::Ch* head = is.PutBegin();// 获取输入流的起始位置
                                    const size_t length = s.Tell()-startOffset;// 返回当前已解析的数值字符串的长度
                                    RAPIDJSON_ASSERT(length<=0xFFFFFFFF);// 确保计算得到的字符串长度不会超过0xFFFFFFFF,即最大为4GB
                                    const typename TargetEncoding::Ch* const str = reinterpret_cast<typename TargetEncoding::Ch*>(head);// 将head指针转换为输出流中的目标编码格式的指针,即将原始数值字符传递给目标编码器处理
                                    cont = handler.RawNumber(str, SizeType(length), false);// 将原始数组字符串传递给回调函数,false表示这是一个原地的字符串
                                }
                                else{// 没有原地解析,则需要复制字符串到目标缓冲区
                                    SizeType numCharsToCopy = static_cast<SizeType>(s.Length());// 获取输入流中剩余的字符数
                                    GenericStringStream<UTF8<NumberCharacter>> srcStream(s.Pop());// 它将使用输入流s.Pop()中的数据
                                    StackStream<typename TargetEncoding::Ch> dstStream(stack_);// 用于将srcStream转换为目标编码类型,stack_是用于目标编码的缓冲区
                                    while(numCharsToCopy--)// 逐个字符从源流srcStream转换并写入目标流dstStream中
                                        Transcoder<UTF8<typename TargetEncoding::Ch>, TargetEncoding>::Transcode(srcStream, dstStream);
                                    dstStream.Put('\0');// 向目标流中写入一个'\0'字符,表示字符串的结束
                                    const typename TargetEncoding::Ch* str = dstStream.Pop();// 从目标流中弹出已转换的字符串,并将其指向str  注意这里返回的不是'\0'字符,这和默认的栈Pop()不同,这个Pop()返回的是栈的起始位置
                                    const SizeType length = static_cast<SizeType>(dstStream.Length())-1;// 计算转换后的字符串长度(去掉结束符)
                                    cont = handler.RawNumber(str, SizeType(length), true);// 将转换后的字符串传递给回调函数,true表示这是一个副本的字符串
                                }
                            }
                            else{// 数字不是作为原始字符串直接处理
                                size_t length = s.Length();// 获取当前流的长度,表示数字字符串的长度
                                const NumberCharacter* decimal = s.Pop();// 获取栈中原数字字符串的起始地址
                                if(useDouble){// 使用double类型来解析数值时,回调Double()
                                    int p = exp+expFrac;// 指数部分和小数部分的合成(小数对应的expFrac就是负数)
                                    if(parseFlags&kParseFullPrecisionFlag)// 尽可能保留精度
                                        d = internal::StrtodFullPrecision(d, p, decimal, length, decimalPosition, exp);
                                    else
                                        d = internal::StrtodNormalPrecision(d, p);
                                    if(d > (std::numeric_limits<double>::max)())// 解析后的浮点数如果超过double的最大值就报错
                                        RAPIDJSON_PARSE_ERROR(kParseErrorNumberTooBig, startOffset);
                                    cont = handler.Double(minus ? -d:d);
                                }
                                else if(useNanOrInf)// 处理NaN或Inf
                                    cont = handler.Double(d);
                                else{// 不使用double,又不处理NaN或Inf
                                    if(use64bit){// 使用了64位整数来解析
                                        if(minus)
                                            cont = handler.Int64(static_cast<int64_t>(~i64+1));
                                        else
                                            cont = handler.Uint64(i64);
                                    }
                                    else{// 32位来解析
                                        if(minus)
                                            cont = handler.Int(static_cast<int32_t>(~i+1));
                                        else
                                            cont = handler.Uint(i);
                                    }
                                }
                            }
                            if (RAPIDJSON_UNLIKELY(!cont))// 回调失败,即可以理解为解析失败
                                RAPIDJSON_PARSE_ERROR(kParseErrorTermination, startOffset);
                        }
                        /**
                         * @brief 根据字符来解析JSON值
                         * 
                         * @tparam parseFlags 
                         * @tparam InputStream 
                         * @tparam Handler 
                         * @param is 
                         * @param handler 
                         */
                        template<unsigned parseFlags, typename InputStream, typename Handler>
                        void ParseValue(InputStream& is, Handler& handler){
                            switch(is.Peek()){
                                case 'n': ParseNull<parseFlags>(is, handler);
                                          break;
                                case 't': ParseTrue<parseFlags>(is, handler);
                                          break;
                                case 'f': ParseFalse<parseFlags>(is, handler);
                                          break;
                                case '"': ParseString<parseFlags>(is, handler);
                                          break;
                                case '{': ParseObject<parseFlags>(is, handler);
                                          break;
                                case '[': ParseArray<parseFlags>(is, handler);
                                          break;
                                default :
                                          ParseNumber<parseFlags>(is, handler);
                                          break;
                            }
                        }
                        /**
                         * @brief 定义解析器的状态机中的各种状态
                         * 
                         */
                        enum IterativeParsingState{
                            IterativeParsingFinishState = 0,// 解析完成的状态
                            IterativeParsingErrorState,// 解析错误的状态
                            IterativeParsingStartState,// 标志解析的起始点
                            // Object states
                            IterativeParsingObjectInitialState,// 对象解析的初始状态
                            IterativeParsingMemberKeyState,// 等待对象的键的状态
                            IterativeParsingMemberValueState,// 等待键的值的状态
                            IterativeParsingObjectFinishState,// 对象解析完成的状态
                            // Array states
                            IterativeParsingArrayInitialState,// 数组解析的初始状态
                            IterativeParsingElementState,// 等待数组元素的状态
                            IterativeParsingArrayFinishState,// 数组解析完成的状态
                            // Single value state
                            IterativeParsingValueState,// 等待解析一个JSON值的状态
                            // 用于解析JSON分隔符
                            IterativeParsingElementDelimiterState,// 等待数组元素之间的逗号的状态
                            IterativeParsingMemberDelimiterState,// 等待对象成员之间的逗号的状态
                            IterativeParsingKeyValueDelimiterState,// 等待键值对之间的冒号的状态
                            cIterativeParsingStateCount// 记录枚举值的总数,用于状态机实现中快速检验合法状态
                        };
                        /**
                         * @brief 定义一个表示JSON文本中的符号或关键字的令牌枚举类
                         * 其实就是一个字符对应的标记
                         */
                        enum Token{
                            // 括号令牌
                            LeftBracketToken = 0,// [
                            RightBracketToken,// ]
                            LeftCurlyBracketToken,// {
                            RightCurlyBracketToken,// }
                            // 分隔符令牌
                            CommaToken,// ,
                            ColonToken,// :
                            // 数据类型的令牌
                            StringToken,// 字符串
                            FalseToken,// false
                            TrueToken,// true
                            NullToken,// null
                            NumberToken,// 数字
                            // 令牌总数
                            kTokenCount// 记录令牌的总数,方便在状态机实现中快速校验合法令牌
                        };
                        /**
                         * @brief 将单个字符 (c) 映射为对应的令牌 (Token) 类型
                         * 
                         * @param c 
                         * @return RAPIDJSON_FORCEINLINE 
                         */
                        RAPIDJSON_FORCEINLINE Token Tokenize(Ch c) const {
                            //!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
                            #define N NumberToken
                            #define N16 N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N
                                    // ASCII码到令牌的映射表
                                    static const unsigned char tokenMap[256] = {
                                        N16, // 00~0F
                                        N16, // 10~1F
                                        N, N, StringToken, N, N, N, N, N, N, N, N, N, CommaToken, N, N, N, // 20~2F
                                        N, N, N, N, N, N, N, N, N, N, ColonToken, N, N, N, N, N, // 30~3F
                                        N16, // 40~4F
                                        N, N, N, N, N, N, N, N, N, N, N, LeftBracketToken, N, RightBracketToken, N, N, // 50~5F
                                        N, N, N, N, N, N, FalseToken, N, N, N, N, N, N, N, NullToken, N, // 60~6F
                                        N, N, N, N, TrueToken, N, N, N, N, N, N, LeftCurlyBracketToken, N, RightCurlyBracketToken, N, N, // 70~7F
                                        N16, N16, N16, N16, N16, N16, N16, N16 // 80~FF
                                    };
                            #undef N
                            #undef N16
                            //!@endcond
                            if(sizeof(Ch)==1||static_cast<unsigned>(c)<256)
                                return static_cast<Token>(tokenMap[static_cast<unsigned char>(c)]);
                            else
                                return NumberToken;
                        }
                        /**
                         * @brief 基于当前的解析状态和令牌类型,预测下一步的解析状态.使用了一个预定义的状态转移表G
                         * 
                         * @param state 
                         * @param token 
                         * @return RAPIDJSON_FORCEINLINE 
                         */
                        RAPIDJSON_FORCEINLINE IterativeParsingState Predict(IterativeParsingState state, Token token) const {
                            // G[state][token]:表示当前状态state遇到令牌token时的下一个状态
                            static const char G[cIterativeParsingStateCount][kTokenCount] = {
                            // Finish(sink state)
                            {
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState
                            },
                            // Error(sink state)
                            {
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState
                            },
                            // Start
                            {
                                IterativeParsingArrayInitialState,  // Left bracket
                                IterativeParsingErrorState,         // Right bracket
                                IterativeParsingObjectInitialState, // Left curly bracket
                                IterativeParsingErrorState,         // Right curly bracket
                                IterativeParsingErrorState,         // Comma
                                IterativeParsingErrorState,         // Colon
                                IterativeParsingValueState,         // String
                                IterativeParsingValueState,         // False
                                IterativeParsingValueState,         // True
                                IterativeParsingValueState,         // Null
                                IterativeParsingValueState          // Number
                            },
                            // ObjectInitial
                            {
                                IterativeParsingErrorState,         // Left bracket
                                IterativeParsingErrorState,         // Right bracket
                                IterativeParsingErrorState,         // Left curly bracket
                                IterativeParsingObjectFinishState,  // Right curly bracket
                                IterativeParsingErrorState,         // Comma
                                IterativeParsingErrorState,         // Colon
                                IterativeParsingMemberKeyState,     // String
                                IterativeParsingErrorState,         // False
                                IterativeParsingErrorState,         // True
                                IterativeParsingErrorState,         // Null
                                IterativeParsingErrorState          // Number
                            },
                            // MemberKey
                            {
                                IterativeParsingErrorState,             // Left bracket
                                IterativeParsingErrorState,             // Right bracket
                                IterativeParsingErrorState,             // Left curly bracket
                                IterativeParsingErrorState,             // Right curly bracket
                                IterativeParsingErrorState,             // Comma
                                IterativeParsingKeyValueDelimiterState, // Colon
                                IterativeParsingErrorState,             // String
                                IterativeParsingErrorState,             // False
                                IterativeParsingErrorState,             // True
                                IterativeParsingErrorState,             // Null
                                IterativeParsingErrorState              // Number
                            },
                            // MemberValue
                            {
                                IterativeParsingErrorState,             // Left bracket
                                IterativeParsingErrorState,             // Right bracket
                                IterativeParsingErrorState,             // Left curly bracket
                                IterativeParsingObjectFinishState,      // Right curly bracket
                                IterativeParsingMemberDelimiterState,   // Comma
                                IterativeParsingErrorState,             // Colon
                                IterativeParsingErrorState,             // String
                                IterativeParsingErrorState,             // False
                                IterativeParsingErrorState,             // True
                                IterativeParsingErrorState,             // Null
                                IterativeParsingErrorState              // Number
                            },
                            // ObjectFinish(sink state)
                            {
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState
                            },
                            // ArrayInitial
                            {
                                IterativeParsingArrayInitialState,      // Left bracket(push Element state)
                                IterativeParsingArrayFinishState,       // Right bracket
                                IterativeParsingObjectInitialState,     // Left curly bracket(push Element state)
                                IterativeParsingErrorState,             // Right curly bracket
                                IterativeParsingErrorState,             // Comma
                                IterativeParsingErrorState,             // Colon
                                IterativeParsingElementState,           // String
                                IterativeParsingElementState,           // False
                                IterativeParsingElementState,           // True
                                IterativeParsingElementState,           // Null
                                IterativeParsingElementState            // Number
                            },
                            // Element
                            {
                                IterativeParsingErrorState,             // Left bracket
                                IterativeParsingArrayFinishState,       // Right bracket
                                IterativeParsingErrorState,             // Left curly bracket
                                IterativeParsingErrorState,             // Right curly bracket
                                IterativeParsingElementDelimiterState,  // Comma
                                IterativeParsingErrorState,             // Colon
                                IterativeParsingErrorState,             // String
                                IterativeParsingErrorState,             // False
                                IterativeParsingErrorState,             // True
                                IterativeParsingErrorState,             // Null
                                IterativeParsingErrorState              // Number
                            },
                            // ArrayFinish(sink state)
                            {
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState
                            },
                            // Single Value (sink state)
                            {
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                                IterativeParsingErrorState
                            },
                            // ElementDelimiter
                            {
                                IterativeParsingArrayInitialState,      // Left bracket(push Element state)
                                IterativeParsingArrayFinishState,       // Right bracket
                                IterativeParsingObjectInitialState,     // Left curly bracket(push Element state)
                                IterativeParsingErrorState,             // Right curly bracket
                                IterativeParsingErrorState,             // Comma
                                IterativeParsingErrorState,             // Colon
                                IterativeParsingElementState,           // String
                                IterativeParsingElementState,           // False
                                IterativeParsingElementState,           // True
                                IterativeParsingElementState,           // Null
                                IterativeParsingElementState            // Number
                            },
                            // MemberDelimiter
                            {
                                IterativeParsingErrorState,         // Left bracket
                                IterativeParsingErrorState,         // Right bracket
                                IterativeParsingErrorState,         // Left curly bracket
                                IterativeParsingObjectFinishState,  // Right curly bracket
                                IterativeParsingErrorState,         // Comma
                                IterativeParsingErrorState,         // Colon
                                IterativeParsingMemberKeyState,     // String
                                IterativeParsingErrorState,         // False
                                IterativeParsingErrorState,         // True
                                IterativeParsingErrorState,         // Null
                                IterativeParsingErrorState          // Number
                            },
                            // KeyValueDelimiter
                            {
                                IterativeParsingArrayInitialState,      // Left bracket(push MemberValue state)
                                IterativeParsingErrorState,             // Right bracket
                                IterativeParsingObjectInitialState,     // Left curly bracket(push MemberValue state)
                                IterativeParsingErrorState,             // Right curly bracket
                                IterativeParsingErrorState,             // Comma
                                IterativeParsingErrorState,             // Colon
                                IterativeParsingMemberValueState,       // String
                                IterativeParsingMemberValueState,       // False
                                IterativeParsingMemberValueState,       // True
                                IterativeParsingMemberValueState,       // Null
                                IterativeParsingMemberValueState        // Number
                            },
                            }; // End of G
                            return static_cast<IterativeParsingState>(G[state][token]);// 根据当前状态和令牌,查询状态转移表G,进而预测下一步状态
                        }
                        /**
                         * @brief 状态转移由Predict()和Transit()完成.Predict()确定目标状态dst,Transit()完成从src到dst的状态转移,并执行必要的解析操作
                         * Transit()是基于有限状态机设计的:上一状态src+当前令牌token=>要转移的目标状态dst
                         * @tparam parseFlags 
                         * @tparam InputStream 
                         * @tparam Handler 
                         * @param src 当前解析状态,也就是状态机中的上一状态
                         * @param token 当前输入的JSON令牌(如{、[、:等)
                         * @param dst 预测的下一个解析状态
                         * @param is 输入流实例
                         * @param handler 用户定义的处理器对象,用于执行StartObject()、EndArray()等
                         * @return RAPIDJSON_FORCEINLINE 返回新的解析状态
                         */
                        template<unsigned parseFlags, typename InputStream, typename Handler>
                        RAPIDJSON_FORCEINLINE IterativeParsingState Transit(IterativeParsingState src, Token token, IterativeParsingState dst, InputStream& is, Handler& handler){
                            (void)token;
                            switch(dst){
                                case IterativeParsingErrorState:// 目标状态是错误状态,无需任何额外操作,直接返回
                                    return dst;
                                // 当前目标状态是对象初始状态或数组初始状态(对于JSON的{和[)
                                case IterativeParsingObjectInitialState:
                                case IterativeParsingArrayInitialState:
                                {
                                    IterativeParsingState n = src;// n用于记录当前解析状态  !!!用于后续在嵌套结束时恢复
                                    if(src==IterativeParsingElementState||src==IterativeParsingElementDelimiterState)// 上一个状态为解析一个数组的元素或者刚处理完一个数组元素的分隔符时表示当前正在正在解析数组下一个元素
                                        n = IterativeParsingElementState;
                                    else if(src==IterativeParsingKeyValueDelimiterState)// 刚处理完键值对的分隔符则下一个需要解析的是键值对的Value,即目标状态虽然是IterativeParsingObjectInitialState,但是此时是在上一层的键值对的值的位置开始一个嵌套新的对象.这里n是用于在嵌套结束时恢复,即嵌套结束,这里其实就是键值对的值的解析状态
                                        n = IterativeParsingMemberValueState;
                                    *stack_.template Push<SizeType>(1) = n;// 将n保存到解析栈中
                                    *stack_.template Push<SizeType>(1) = 0;// 栈中保存当前对象或数组的成员/元素计数,初始为0.计数会在解析过程更新
                                    bool hr = (dst==IterativeParsingObjectInitialState) ? handler.StartObject():handler.StartArray();// 根据目标状态决定调用的回调函数
                                    if(!hr){
                                        RAPIDJSON_PARSE_ERROR_NORETURN(kParseErrorTermination, is.Tell());
                                        return IterativeParsingErrorState;
                                    }
                                    else{
                                        is.Take();// 消耗当前字符[或{
                                        return dst;// 返回目标状态dst(对象初始状态或数组初始状态)
                                    }
                                }
                                case IterativeParsingMemberKeyState:// 解析对象成员的键
                                    ParseString<parseFlags>(is, handler, true);
                                    if(HasParseError())
                                        return IterativeParsingErrorState;
                                    else
                                        return dst;
                                case IterativeParsingKeyValueDelimiterState:// 处理对象成员中键值对的分隔符":"
                                    RAPIDJSON_ASSERT(token==ColonToken);
                                    is.Take();
                                    return dst;
                                case IterativeParsingMemberValueState:// 解析对象成员的值
                                    ParseValue<parseFlags>(is, handler);
                                    if(HasParseError())
                                        return IterativeParsingErrorState;
                                    return dst;
                                case IterativeParsingElementState:// 解析数组中的元素
                                    ParseValue<parseFlags>(is, handler);
                                    if(HasParseError())
                                        return IterativeParsingErrorState;
                                    return dst;
                                // 处理对象成员之间的逗号和数组元素之间的逗号
                                case IterativeParsingMemberDelimiterState:
                                case IterativeParsingElementDelimiterState:
                                    is.Take();// 消耗逗号这个字符
                                    *stack_.template Top<SizeType>() = *stack_.template Top<SizeType>()+1;// 将当前对象的成员数或数组的元素数+1并记录(加1就是表示一个完整的对象成员或数组元素结束)
                                    return dst;
                                case IterativeParsingObjectFinishState:// 处理对象解析结束的情况(})
                                {
                                    if(!(parseFlags&kParseTrailingCommasFlag)&&src==IterativeParsingMemberDelimiterState){// 上一个状态是IterativeParsingMemberDelimiterState表示对象的成员中存在非法的尾随逗号,此时}要直接报错
                                        RAPIDJSON_PARSE_ERROR_NORETURN(kParseErrorObjectMissName, is.Tell());
                                        return IterativeParsingErrorState;
                                    }
                                    SizeType c = *stack_.template Pop<SizeType>(1);// 从栈中弹出计数器,记录当前对象的成员数
                                    if(src==IterativeParsingMemberValueState)// 如果上一个状态是IterativeParsingMemberValueState,表示对象最后一个成员的值解析完毕,需要将成员数+1
                                        ++c;
                                    IterativeParsingState n = static_cast<IterativeParsingState>(*stack_.template Pop<SizeType>(1));// 弹出之前压入的状态(通常是进入该对象前的状态)
                                    if(n==IterativeParsingStartState)// 如果弹出的状态是IterativeParsingStartState,说明这是最外层的对象,解析已完成,将状态设为IterativeParsingFinishState
                                        n = IterativeParsingFinishState;
                                    bool hr = handler.EndObject(c);// 通知处理器对象解析结束,并传递对象的成员数量c
                                    if(!hr){
                                        RAPIDJSON_PARSE_ERROR_NORETURN(kParseErrorTermination, is.Tell());
                                        return IterativeParsingErrorState;
                                    }
                                    else{
                                        is.Take();// 消耗结束字符}
                                        return n;// 返回上一步状态
                                    }
                                }
                                default:
                                    RAPIDJSON_ASSERT(dst==IterativeParsingValueState);
                                    ParseValue<parseFlags>(is, handler);
                                    if(HasParseError())
                                        return IterativeParsingErrorState;
                                    return IterativeParsingFinishState;
                            }
                        }
                        /**
                         * @brief 根据当前解析状态src和输入流设置适当的解析错误
                         * 
                         * @tparam InputStream 
                         * @param src 
                         * @param is 
                         */
                        template<typename InputStream>
                        void HandleError(IterativeParsingState src, InputStream& is){
                            if(HasParseError())// 已经有错误标志则直接返回
                                return;
                            switch(src){// 根据当前解析状态选择适当的错误类型
                                case IterativeParsingStartState:            RAPIDJSON_PARSE_ERROR(kParseErrorDocumentEmpty, is.Tell()); return;
                                case IterativeParsingFinishState:           RAPIDJSON_PARSE_ERROR(kParseErrorDocumentRootNotSingular, is.Tell()); return;
                                case IterativeParsingObjectInitialState:
                                case IterativeParsingMemberDelimiterState:  RAPIDJSON_PARSE_ERROR(kParseErrorObjectMissName, is.Tell()); return;
                                case IterativeParsingMemberKeyState:        RAPIDJSON_PARSE_ERROR(kParseErrorObjectMissColon, is.Tell()); return;
                                case IterativeParsingMemberValueState:      RAPIDJSON_PARSE_ERROR(kParseErrorObjectMissCommaOrCurlyBracket, is.Tell()); return;
                                case IterativeParsingKeyValueDelimiterState:
                                case IterativeParsingArrayInitialState:
                                case IterativeParsingElementDelimiterState: RAPIDJSON_PARSE_ERROR(kParseErrorValueInvalid, is.Tell()); return;
                                default: RAPIDJSON_ASSERT(src == IterativeParsingElementState); RAPIDJSON_PARSE_ERROR(kParseErrorArrayMissCommaOrSquareBracket, is.Tell()); return;
                            }
                        }
                        // 判断给定状态是否是分隔符相关的解析状态
                        RAPIDJSON_FORCEINLINE bool IsIterativeParsingDelimiterState(IterativeParsingState s) const {
                            return s >= IterativeParsingElementDelimiterState;// 大于等于IterativeParsingElementDelimiterState的状态就是与分隔符相关的状态,从状态映射表IterativeParsingState中可以看出
                        }
                        // 判断当前状态是否是结束状态
                        RAPIDJSON_FORCEINLINE bool IsIterativeParsingCompleteState(IterativeParsingState s) const {
                            return s <= IterativeParsingErrorState;
                        }
                        /**
                         * @brief 实现迭代式JSON文档解析逻辑
                         * 
                         * @tparam parseFlags 
                         * @tparam InputStream 
                         * @tparam Handler 
                         * @param is 
                         * @param handler 
                         * @return ParseResult 
                         */
                        template<unsigned parseFlags, typename InputStream, typename Handler>
                        ParseResult IterativeParse(InputStream& is, Handler& handler){
                            parseResult_.Clear();// 清空之前的解析结果
                            ClearStackOnExit scope(*this);// 确保在当前对象退出时清空栈内容,防止内存泄漏(类似RAII机制)
                            IterativeParsingState state = IterativeParsingStartState;// 解析刚开始
                            SkipWhitespaceAndComments<parseFlags>(is);
                            RAPIDJSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
                            while(is.Peek()!='\0'){// 输入流未到文件末尾,持续解析
                                Token t = Tokenize(is.Peek());// 返回当前字符对应的token令牌
                                IterativeParsingState n = Predict(state, t);// 根据当前状态和令牌预测出下一个解析状态
                                IterativeParsingState d = Transit<parseFlags>(state, t, n, is, handler);// 根据当前状态、令牌和预测状态,执行状态转换,返回转换后的状态
                                if(d==IterativeParsingErrorState){// 错误状态处理
                                    HandleError(state, is);
                                    break;
                                }
                                state = d;// 将转换后的状态更新为当前状态,为下一轮解析做准备
                                if((parseFlags&kParseStopWhenDoneFlag)&&state==IterativeParsingFinishState)// 启用了kParseStopWhenDoneFlag,且文档解析完成就在这提前结束解析
                                    break;
                                SkipWhitespaceAndComments<parseFlags>(is);
                                RAPIDJSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
                            }
                            if(state!=IterativeParsingFinishState)// 文档解析结束时如果状态不是IterativeParsingFinishState,就报错
                                HandleError(state, is);
                            return parseResult_;
                        }
                        static const size_t kDefaultStackCapacity = 256;   
                        internal::Stack<StackAllocator> stack_;// 用于存储解析过程中的临时数据
                        ParseResult parseResult_;// 保存解析结果,包括成功或错误的标志及错误位置
                        IterativeParsingState state_;
                    };
    typedef GenericReader<UTF8<>, UTF8<>> Reader;
}
#endif
