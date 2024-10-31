#ifndef RAPIDJSON_RAPIDJSON_H_
#define RAPIDJSON_RAPIDJSON_H_

#include <cstdlib>// malloc(), realloc(), free(), size_t
#include <cstring>// memset(), memcpy(), memmove(), memcmp()

//!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
#define RAPIDJSON_STRINGIFY(x) RAPIDJSON_DO_STRINGIFY(x)
#define RAPIDJSON_DO_STRINGIFY(x) #x

#define RAPIDJSON_JOIN(X, Y) RAPIDJSON_DO_JOIN(X, Y)
#define RAPIDJSON_DO_JOIN(X, Y) RAPIDJSON_DO_JOIN2(X, Y)
#define RAPIDJSON_DO_JOIN2(X, Y) X##Y
//!@endcond

#define RAPIDJSON_MAJOR_VERSION 1
#define RAPIDJSON_MINOR_VERSION 1
#define RAPIDJSON_PATCH_VERSION 0
#define RAPIDJSON_VERSION_STRING \
    RAPIDJSON_STRINGIFY(RAPIDJSON_MAJOR_VERSION.RAPIDJSON_MINOR_VERSION.RAPIDJSON_PATCH_VERSION)

//!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
#if defined(_MSC_VER)
#define RAPIDJSON_CPLUSPLUS _MSVC_LANG
#else
#define RAPIDJSON_CPLUSPLUS __cplusplus
#endif
//!@endcond

// RAPIDJSON_HAS_STDSTRING用于控制是否支持std::string,默认不支持
#ifndef RAPIDJSON_HAS_STDSTRING
#ifdef RAPIDJSON_DOXYGEN_RUNNING
#define RAPIDJSON_HAS_STDSTRING 1// force generation of documentation
#else
#define RAPIDJSON_HAS_STDSTRING 0// no std::string support by default
#endif
#endif
#if RAPIDJSON_HAS_STDSTRING
#include <string>
#endif// RAPIDJSON_HAS_STDSTRING

#ifndef RAPIDJSON_USE_MEMBERSMAP
#define RAPIDJSON_USE_MEMBERSMAP 0 
#endif

#ifndef RAPIDJSON_NO_INT64DEFINE
//!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
#if defined(_MSC_VER) && (_MSC_VER < 1800)// 检查是否是 Microsoft Visual C++ 编译器,并且版本是否低于 2013
#include "msinttypes/stdint.h"
#include "msinttypes/inttypes.h"
#else
// 其它编译器之间包含下面的标准头文件即可
#include <stdint.h>
#include <inttypes.h>
#endif
//!@endcond
#ifdef RAPIDJSON_DOXYGEN_RUNNING// 如果正在生成 Doxygen 文档,则主动定义 RAPIDJSON_NO_INT64DEFINE,以避免在文档中出现不必要的内容
#define RAPIDJSON_NO_INT64DEFINE
#endif
#endif// RAPIDJSON_NO_INT64TYPEDEF

// 根据不同编译器和调试标志(NDEBUG)来指定强制内联指令,从而优化性能
#ifndef RAPIDJSON_FORCEINLINE
//!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
#if defined(_MSC_VER) && defined(NDEBUG)// 检查是否在 Microsoft Visual C++ 编译器环境中,并且未定义调试模式
#define RAPIDJSON_FORCEINLINE __forceinline
#elif defined(__GNUC__) && __GNUC__ >= 4 && defined(NDEBUG)// 检查是否是 GCC 编译器,且版本为 4 或更高,并且未定义调试模式
#define RAPIDJSON_FORCEINLINE __attribute__((always_inline))
#else
#define RAPIDJSON_FORCEINLINE// 以上都不满足,则不使用任何内联指令
#endif
//!@endcond
#endif// RAPIDJSON_FORCEINLINE

#define RAPIDJSON_LITTLEENDIAN  0   // 小端序值为0
#define RAPIDJSON_BIGENDIAN     1   // 大端序值为1

#ifndef RAPIDJSON_ENDIAN
// 基于GCC4.6的__BYTE_ORDER__检查
#  ifdef __BYTE_ORDER__
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#      define RAPIDJSON_ENDIAN RAPIDJSON_LITTLEENDIAN
#    elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#      define RAPIDJSON_ENDIAN RAPIDJSON_BIGENDIAN
#    else
#      error Unknown machine endianness detected. User needs to define RAPIDJSON_ENDIAN.
#    endif// __BYTE_ORDER__
// 基于GLIBC的<endian.h>检查
#  elif defined(__GLIBC__)
#    include <endian.h>
#    if (__BYTE_ORDER == __LITTLE_ENDIAN)
#      define RAPIDJSON_ENDIAN RAPIDJSON_LITTLEENDIAN
#    elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define RAPIDJSON_ENDIAN RAPIDJSON_BIGENDIAN
#    else
#      error Unknown machine endianness detected. User needs to define RAPIDJSON_ENDIAN.
#   endif // __GLIBC__
// 基于系统宏_LITTLE_ENDIAN和_BIG_ENDIAN检查
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define RAPIDJSON_ENDIAN RAPIDJSON_LITTLEENDIAN
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define RAPIDJSON_ENDIAN RAPIDJSON_BIGENDIAN
// 使用特定的处理器架构宏来确定字节序
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__ppc64__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#    define RAPIDJSON_ENDIAN RAPIDJSON_BIGENDIAN
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define RAPIDJSON_ENDIAN RAPIDJSON_LITTLEENDIAN
#  elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#    define RAPIDJSON_ENDIAN RAPIDJSON_LITTLEENDIAN
#  elif defined(RAPIDJSON_DOXYGEN_RUNNING)// 如果在运行 Doxygen 文档生成工具,不定义 RAPIDJSON_ENDIAN,以便在生成的文档中显示为未定义
#    define RAPIDJSON_ENDIAN
#  else
#    error Unknown machine endianness detected. User needs to define RAPIDJSON_ENDIAN.
#  endif
#endif// RAPIDJSON_ENDIAN

// 检测当前平台是否支持64位 __LP64__、__x86_64__、__ILP32__、_WIN64是常见的64位架构
#ifndef RAPIDJSON_64BIT
#if defined(__LP64__) || (defined(__x86_64__) && defined(__ILP32__)) || defined(_WIN64) || defined(__EMSCRIPTEN__)
#define RAPIDJSON_64BIT 1
#else
#define RAPIDJSON_64BIT 0
#endif
#endif// RAPIDJSON_64BIT 

// 确保传入的地址x对齐到8字节边界
#ifndef RAPIDJSON_ALIGN
#define RAPIDJSON_ALIGN(x) (((x) + static_cast<size_t>(7u)) & ~static_cast<size_t>(7u))
#endif

// 支持通过传入两个32位数(高32位和低32位)来生成一个64位常量.这对于不支持直接写64位常量的编译器很有用
#ifndef RAPIDJSON_UINT64_C2
#define RAPIDJSON_UINT64_C2(high32, low32) ((static_cast<uint64_t>(high32) << 32) | static_cast<uint64_t>(low32))
#endif

// 定义用于在x86_64架构上减少指针大小的存储需求的48位指针优化
#ifndef RAPIDJSON_48BITPOINTER_OPTIMIZATION
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define RAPIDJSON_48BITPOINTER_OPTIMIZATION 1
#else
#define RAPIDJSON_48BITPOINTER_OPTIMIZATION 0
#endif
#endif// RAPIDJSON_48BITPOINTER_OPTIMIZATION 

// 如果开启了48位指针优化,则通过位运算的方式对指针进行压缩;否则,直接赋值
#if RAPIDJSON_48BITPOINTER_OPTIMIZATION == 1
#if RAPIDJSON_64BIT != 1
#error RAPIDJSON_48BITPOINTER_OPTIMIZATION can only be set to 1 when RAPIDJSON_64BIT=1
#endif
#define RAPIDJSON_SETPOINTER(type, p, x) (p = reinterpret_cast<type *>((reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(RAPIDJSON_UINT64_C2(0xFFFF0000, 0x00000000))) | reinterpret_cast<uintptr_t>(reinterpret_cast<const void*>(x))))
#define RAPIDJSON_GETPOINTER(type, p) (reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(RAPIDJSON_UINT64_C2(0x0000FFFF, 0xFFFFFFFF))))
#else
#define RAPIDJSON_SETPOINTER(type, p, x) (p = (x))
#define RAPIDJSON_GETPOINTER(type, p) (p)
#endif

// 检查是否支持SIMD
#if defined(RAPIDJSON_SSE2) || defined(RAPIDJSON_SSE42) \
    || defined(RAPIDJSON_NEON) || defined(RAPIDJSON_DOXYGEN_RUNNING)
#define RAPIDJSON_SIMD
#endif

#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#ifdef RAPIDJSON_DOXYGEN_RUNNING
#define RAPIDJSON_NO_SIZETYPEDEFINE
#endif

namespace RAPIDJSON{
    typedef unsigned SizeType;
}
#endif

namespace RAPIDJSON{
    using std::size_t;
}

// 运行时断言宏  <cassert>中的assert()
#ifndef RAPIDJSON_ASSERT
#include <cassert>
#define RAPIDJSON_ASSERT(x) assert(x)
#endif// RAPIDJSON_ASSERT 

// 使用C++11的static_assert来实现编译器断言
#ifndef RAPIDJSON_STATIC_ASSERT
#if RAPIDJSON_CPLUSPLUS >= 201103L || ( defined(_MSC_VER) && _MSC_VER >= 1800 )
#define RAPIDJSON_STATIC_ASSERT(x) \
   static_assert(x, RAPIDJSON_STRINGIFY(x))
#endif 
#endif// RAPIDJSON_STATIC_ASSERT


// 为了兼容C++03,定义了一个Boost风格的C++03静态断言实现
#ifndef RAPIDJSON_STATIC_ASSERT
#ifndef __clang__
//!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
#endif
namespace RAPIDJSON{
    template <bool x> struct STATIC_ASSERTION_FAILURE;
    template <> struct STATIC_ASSERTION_FAILURE<true> { enum { value = 1 }; };
    template <size_t x> struct StaticAssertTest {};
}
#if defined(__GNUC__) || defined(__clang__)
#define RAPIDJSON_STATIC_ASSERT_UNUSED_ATTRIBUTE __attribute__((unused))
#else
#define RAPIDJSON_STATIC_ASSERT_UNUSED_ATTRIBUTE 
#endif
#ifndef __clang__
//!@endcond
#endif

// 将静态断言定义为一个类型别名
#define RAPIDJSON_STATIC_ASSERT(x) \
    typedef ::RAPIDJSON::StaticAssertTest< \
      sizeof(::RAPIDJSON::STATIC_ASSERTION_FAILURE<bool(x) >)> \
    RAPIDJSON_JOIN(StaticAssertTypedef, __LINE__) RAPIDJSON_STATIC_ASSERT_UNUSED_ATTRIBUTE
#endif// RAPIDJSON_STATIC_ASSERT

// 使用GCC和Clang的__builtin_expect内建函数,提示编译器分支预测中的常见情况,从而提高CPU分支预测效率
// RAPIDJSON_LIKELY(x)大概率为true
#ifndef RAPIDJSON_LIKELY
#if defined(__GNUC__) || defined(__clang__)
#define RAPIDJSON_LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define RAPIDJSON_LIKELY(x) (x)
#endif
#endif
// RAPIDJSON_UNLIKELY(x)大概率为false
#ifndef RAPIDJSON_UNLIKELY
#if defined(__GNUC__) || defined(__clang__)
#define RAPIDJSON_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define RAPIDJSON_UNLIKELY(x) (x)
#endif
#endif

//!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN
#define RAPIDJSON_MULTILINEMACRO_BEGIN do {
#define RAPIDJSON_MULTILINEMACRO_END \
} while((void)0, 0)

#define RAPIDJSON_VERSION_CODE(x,y,z) \
  (((x)*100000) + ((y)*100) + (z))

// 如果编译器支持__has_builtin宏,就可以用来检测特定内建函数的存在
#if defined(__has_builtin)
#define RAPIDJSON_HAS_BUILTIN(x) __has_builtin(x)
#else
#define RAPIDJSON_HAS_BUILTIN(x) 0
#endif

// 检测GCC编译器版本
#if defined(__GNUC__)
#define RAPIDJSON_GNUC \
    RAPIDJSON_VERSION_CODE(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)
#endif

// Clang和GCC支持_Pragma语法来生成编译器指令,如关闭或开启特定警告
#if defined(__clang__) || (defined(RAPIDJSON_GNUC) && RAPIDJSON_GNUC >= RAPIDJSON_VERSION_CODE(4,2,0))
#define RAPIDJSON_PRAGMA(x) _Pragma(RAPIDJSON_STRINGIFY(x))
#define RAPIDJSON_DIAG_PRAGMA(x) RAPIDJSON_PRAGMA(GCC diagnostic x)
#define RAPIDJSON_DIAG_OFF(x) \
    RAPIDJSON_DIAG_PRAGMA(ignored RAPIDJSON_STRINGIFY(RAPIDJSON_JOIN(-W,x)))

// Clang和GCC  将诊断状态推入栈中和栈中弹出之前保存的诊断状态
#if defined(__clang__) || (defined(RAPIDJSON_GNUC) && RAPIDJSON_GNUC >= RAPIDJSON_VERSION_CODE(4,6,0))
#define RAPIDJSON_DIAG_PUSH RAPIDJSON_DIAG_PRAGMA(push)
#define RAPIDJSON_DIAG_POP  RAPIDJSON_DIAG_PRAGMA(pop)
#else
#define RAPIDJSON_DIAG_PUSH 
#define RAPIDJSON_DIAG_POP 
#endif

// 在 MSVC 上,通过 __pragma 语法设置 MSVC 编译器的警告控制
#elif defined(_MSC_VER)
// pragma (MSVC specific)
#define RAPIDJSON_PRAGMA(x) __pragma(x)
#define RAPIDJSON_DIAG_PRAGMA(x) RAPIDJSON_PRAGMA(warning(x))
#define RAPIDJSON_DIAG_OFF(x) RAPIDJSON_DIAG_PRAGMA(disable: x)
#define RAPIDJSON_DIAG_PUSH RAPIDJSON_DIAG_PRAGMA(push)
#define RAPIDJSON_DIAG_POP  RAPIDJSON_DIAG_PRAGMA(pop)
#else
#define RAPIDJSON_DIAG_OFF(x)
#define RAPIDJSON_DIAG_PUSH   
#define RAPIDJSON_DIAG_POP  
#endif

// 检测是否支持C++11
#ifndef RAPIDJSON_HAS_CXX11
#define RAPIDJSON_HAS_CXX11 (RAPIDJSON_CPLUSPLUS >= 201103L)
#endif
// 检测是否支持C++11中的右值引用
#ifndef RAPIDJSON_HAS_CXX11_RVALUE_REFS
#if RAPIDJSON_HAS_CXX11
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1
#elif defined(__clang__)
#if __has_feature(cxx_rvalue_references) && \
    (defined(_MSC_VER) || defined(_LIBCPP_VERSION) || defined(__GLIBCXX__) && __GLIBCXX__ >= 20080306)
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1
#else
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 0
#endif
#elif (defined(RAPIDJSON_GNUC) && (RAPIDJSON_GNUC >= RAPIDJSON_VERSION_CODE(4,3,0)) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || \
      (defined(_MSC_VER) && _MSC_VER >= 1600) || \
      (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x5140 && defined(__GXX_EXPERIMENTAL_CXX0X__))
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1
#else
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 0
#endif
#endif// RAPIDJSON_HAS_CXX11_RVALUE_REFS

#if RAPIDJSON_HAS_CXX11_RVALUE_REFS
#include <utility>// std::move
#endif

// 检查编译器是否支持 noexcept 关键字
#ifndef RAPIDJSON_HAS_CXX11_NOEXCEPT
#if RAPIDJSON_HAS_CXX11
#define RAPIDJSON_HAS_CXX11_NOEXCEPT 1
#elif defined(__clang__)
#define RAPIDJSON_HAS_CXX11_NOEXCEPT __has_feature(cxx_noexcept)
#elif (defined(RAPIDJSON_GNUC) && (RAPIDJSON_GNUC >= RAPIDJSON_VERSION_CODE(4,6,0)) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || \
    (defined(_MSC_VER) && _MSC_VER >= 1900) || \
    (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x5140 && defined(__GXX_EXPERIMENTAL_CXX0X__))
#define RAPIDJSON_HAS_CXX11_NOEXCEPT 1
#else
#define RAPIDJSON_HAS_CXX11_NOEXCEPT 0
#endif
#endif
#ifndef RAPIDJSON_NOEXCEPT
#if RAPIDJSON_HAS_CXX11_NOEXCEPT
#define RAPIDJSON_NOEXCEPT noexcept
#else
#define RAPIDJSON_NOEXCEPT throw()
#endif// RAPIDJSON_HAS_CXX11_NOEXCEPT
#endif

// 检测是否支持 C++11 的类型特征(type traits)
#ifndef RAPIDJSON_HAS_CXX11_TYPETRAITS
#if (defined(_MSC_VER) && _MSC_VER >= 1700)
#define RAPIDJSON_HAS_CXX11_TYPETRAITS 1
#else
#define RAPIDJSON_HAS_CXX11_TYPETRAITS 0
#endif// RAPIDJSON_HAS_CXX11_TYPETRAITS
#endif

// 检查编译器是否支持范围 for 循环
#ifndef RAPIDJSON_HAS_CXX11_RANGE_FOR
#if defined(__clang__)
#define RAPIDJSON_HAS_CXX11_RANGE_FOR __has_feature(cxx_range_for)
#elif (defined(RAPIDJSON_GNUC) && (RAPIDJSON_GNUC >= RAPIDJSON_VERSION_CODE(4,6,0)) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || \
      (defined(_MSC_VER) && _MSC_VER >= 1700) || \
      (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x5140 && defined(__GXX_EXPERIMENTAL_CXX0X__))
#define RAPIDJSON_HAS_CXX11_RANGE_FOR 1
#else
#define RAPIDJSON_HAS_CXX11_RANGE_FOR 0
#endif
#endif// RAPIDJSON_HAS_CXX11_RANGE_FOR

// 检测是否支持C++11
#ifndef RAPIDJSON_HAS_CXX17
#define RAPIDJSON_HAS_CXX17 (RAPIDJSON_CPLUSPLUS >= 201703L)
#endif
// 定义一个用于指示 switch 语句中故意落空的属性,以提高代码可读性
#if RAPIDJSON_HAS_CXX17
# define RAPIDJSON_DELIBERATE_FALLTHROUGH [[fallthrough]]
#elif defined(__has_cpp_attribute)
# if __has_cpp_attribute(clang::fallthrough)
#  define RAPIDJSON_DELIBERATE_FALLTHROUGH [[clang::fallthrough]]
# elif __has_cpp_attribute(fallthrough)
#  define RAPIDJSON_DELIBERATE_FALLTHROUGH __attribute__((fallthrough))
# else
#  define RAPIDJSON_DELIBERATE_FALLTHROUGH
# endif
#else
# define RAPIDJSON_DELIBERATE_FALLTHROUGH
#endif
//!@endcond

// 断言检查
#ifndef RAPIDJSON_NOEXCEPT_ASSERT
#ifdef RAPIDJSON_ASSERT_THROWS
#include <cassert>
#define RAPIDJSON_NOEXCEPT_ASSERT(x) assert(x)
#else
#define RAPIDJSON_NOEXCEPT_ASSERT(x) RAPIDJSON_ASSERT(x)
#endif// RAPIDJSON_ASSERT_THROWS
#endif// RAPIDJSON_NOEXCEPT_ASSERT

// 将malloc() realloc() free()三个内存管理函数用宏进行定义
#ifndef RAPIDJSON_MALLOC
#define RAPIDJSON_MALLOC(size) std::malloc(size)
#endif
#ifndef RAPIDJSON_REALLOC
#define RAPIDJSON_REALLOC(ptr, new_size) std::realloc(ptr, new_size)
#endif
#ifndef RAPIDJSON_FREE
#define RAPIDJSON_FREE(ptr) std::free(ptr)
#endif

// 将new() delete()函数用宏进行定义
#ifndef RAPIDJSON_NEW
#define RAPIDJSON_NEW(TypeName) new TypeName
#endif
#ifndef RAPIDJSON_DELETE
#define RAPIDJSON_DELETE(x) delete x
#endif

// 定义一个用于表示JSON数据的不同类型
namespace RAPIDJSON{
enum Type {
    kNullType = 0,      //!< null
    kFalseType = 1,     //!< false
    kTrueType = 2,      //!< true
    kObjectType = 3,    //!< object
    kArrayType = 4,     //!< array
    kStringType = 5,    //!< string
    kNumberType = 6     //!< number
};
}
#endif// RAPIDJSON_RAPIDJSON_H_
