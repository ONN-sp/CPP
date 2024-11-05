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
    template<typename Encoding=UTF8<>, typename Derived=void>
    struct BaseReaderHandler{
        typedef typename Encoding::Ch Ch;

    
    };

}
#endif