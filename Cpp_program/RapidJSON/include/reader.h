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
// 定义一个空宏，用于特定目的，避免编译器报错
#define RAPIDJSON_NOTHING /* deliberately empty */

// 如果没有定义 RAPIDJSON_PARSE_ERROR_EARLY_RETURN
#ifndef RAPIDJSON_PARSE_ERROR_EARLY_RETURN
// 定义一个宏，用于在解析过程中检测解析错误
#define RAPIDJSON_PARSE_ERROR_EARLY_RETURN(value) \
    RAPIDJSON_MULTILINEMACRO_BEGIN \  
    if (RAPIDJSON_UNLIKELY(HasParseError())) { return value; } \
    RAPIDJSON_MULTILINEMACRO_END // 结束多行宏定义
#endif

// 定义一个宏，用于在解析错误时返回空值
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
    RAPIDJSON_MULTILINEMACRO_END // 结束多行宏定义
#endif

// 如果没有定义 RAPIDJSON_PARSE_ERROR
#ifndef RAPIDJSON_PARSE_ERROR
// 定义一个宏，用于处理解析错误并返回
#define RAPIDJSON_PARSE_ERROR(parseErrorCode, offset) \
    RAPIDJSON_MULTILINEMACRO_BEGIN \ 
    RAPIDJSON_PARSE_ERROR_NORETURN(parseErrorCode, offset); \
    RAPIDJSON_PARSE_ERROR_EARLY_RETURN_VOID; \
    RAPIDJSON_MULTILINEMACRO_END // 结束多行宏定义
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
        bool RawNumber(const Ch* str, SizeType len, bool copy) { return static_cast<Override&>(*this).String(str, len, copy);}// 在启用kParseNumbersAsStringsFlag时被调用,将数字作为字符串处理
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
    }

}
#endif