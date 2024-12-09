#ifndef RAPIDJSON_POINTER_H_
#define RAPIDJSON_POINTER_H_

#include "document.h"
#include "uri.h"
#include "internal/itoa.h"
#include "error/error.h"

namespace RAPIDJSON{
    static const SizeType kPointerInvalidIndex = ~SizeType(0);// 代表GenericPointer::Token中的无效索引
    /**
     * @brief 定义一个用于表示JSON Pointer的类模板,主要用于定位和访问JSON文档中的特定元素
     * JSON Pointer是一种标准化的方式,用于描述JSON数据结构中的路径,可以用于引用JSON对象中的特定成员或数组中的特定元素
     * 
     * @tparam ValueType 
     * @tparam Allocator 
     */
    template<typename ValueType, typename Allocator=CrtAllocator>
    class GenericPointer {
        public:
            typedef typename ValueType::EncodingType EncodingType;
            typedef typename ValueType::Ch Ch;
            typedef GenericUri<ValueType, Allocator> UriType;
            /**
             * @brief Token结构体表示的是JSON Pointer中的一个单独的“令牌”.具体来说,它是用于表示JSON Pointer路径中的每个部分(如对象的键名或数组的索引).每个Token对应于JSON Pointer中的一个组件,并且可以通过它来解析和访问JSON数据结构中的特定位置
             * 
             */
            struct Token {
                const Ch* name;// 令牌名称,即JSON Pointer中当前部分的字符串表示
                SizeType length;// 表示令牌名称的长度,不包括终止的空字符
                SizeType index;// 如果令牌表示的是JSON数组的索引,这个字段就存储该索引的值.如果该令牌表示一个字符串(如JSON对象的键名),则index=kPointerInvalidIndex(如果不等于kPointerInvalidIndex,则为有效的JSON数组的索引)
            };
            // 默认构造函数
            GenericPointer(Allocator* allocator=nullptr)
                : allocator_(allocator),
                  ownAllocator_(),
                  nameBuffer_(),
                  tokens_(),
                  tokenCount_(),
                  parseErrorOffset_(),
                  parseErrorCode_(kPointerParseErrorNone)
                {}
            /**
             * @brief 从一个字符串或URI片段解析得到对应的tokens令牌
             * 
             * @param source 
             * @param allocator 
             */
            GenericPointer(const Ch* source, Allocator* allocator=nullptr)
                : allocator_(allocator),
                  ownAllocator_(),
                  nameBuffer_(),
                  tokens_(),
                  tokenCount_(),
                  parseErrorOffset_(),
                  parseErrorCode_(kPointerParseErrorNone)
                {
                    Parse(source, internal::StrLen(source));
                }
            #if RAPIDJSON_HAS_STDSTRING
                explicit GenericPointer(const std::basic_string<Ch>& source, Allocator* allocator=nullptr)
                    : allocator_(allocator),
                      ownAllocator_(),
                      nameBuffer_(),
                      tokens_(),
                      tokenCount_(),
                      parseErrorOffset_(),
                      parseErrorCode_(kPointerParseErrorNone)
                    {
                        Parse(source.c_str(), source.size());
                    }
            #endif
            GenericPointer(const Ch* source, size_t length, Allocator* allocator=nullptr)
                : allocator_(allocator),
                    ownAllocator_(),
                    nameBuffer_(),
                    tokens_(),
                    tokenCount_(),
                    parseErrorOffset_(),
                    parseErrorCode_(kPointerParseErrorNone)
                {
                    Parse(source, length);
                }
            /**
             * @brief 直接接收已经解析好的tokens数组,而不是从source解析得到令牌tokens
             * 
             * @param tokens 
             * @param tokenCount 
             */
            GenericPointer(const Token* tokens, size_t tokenCount)
                : allocator_(),
                  ownAllocator_(),
                  nameBuffer_(),
                  tokens_(const_cast<Token*>(tokens)),
                  tokenCount_(tokenCount),
                  parseErrorOffset_(),
                  parseErrorCode_(kPointerParseErrorNone)
                {}
            // 拷贝构造函数
            GenericPointer(const GenericPointer& rhs)
                : allocator_(),
                  ownAllocator_(),
                  nameBuffer_(),
                  tokens_(const_cast<Token*>(tokens)),
                  tokenCount_(tokenCount),
                  parseErrorOffset_(),
                  parseErrorCode_(kPointerParseErrorNone)
                {
                    *this = rhs;
                }
            GenericPointer(const GenericPointer& rhs, Allocator* allocator)
                : allocator_(allocator),
                  ownAllocator_(),
                  nameBuffer_(),
                  tokens_(const_cast<Token*>(tokens)),
                  tokenCount_(tokenCount),
                  parseErrorOffset_(),
                  parseErrorCode_(kPointerParseErrorNone)
                {
                    *this = rhs;
                }
            ~GenericPointer() {
                if(nameBuffer_)// nameBuffer_表示使用了自定义分配器用于存储tokens(即此时tokens不是外部直接提供的),此时要释放tokens_的内存
                    Allocator::Free(tokens_);
                RAPIDJSON_DELETE(ownAllocator_);
            }

    };
}

#endif