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
             * @param source 当前对象传入的路径
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
            GenericPointer& operator=(const GenericPointer& rhs) {
                if(this!=rhs){// 禁止自我复制
                    if(nameBuffer_)
                        Allocator::Free(tokens_);
                    tokenCount_ = rhs.tokenCount_;
                    parseErrorOffset_ = rhs.parseErrorOffset_;
                    parseErrorCode_ = rhs.parseErrorCode_;
                    if(rhs.nameBuffer_)
                        CopyFrom(rhs);// 拷贝解析得到的tokens_
                    else {// 直接复制从外部直接传入的tokens
                        tokens_ = rhs.tokens_;
                        nameBuffer_ = 0;
                    }
                }
                return *this;
            }
            /**
             * @brief 交换当前GenericPointer对象与另一个GenericPointer对象的内容
             * 
             * @param other 
             * @return GenericPointer& 
             */
            GenericPointer& Swap(GenericPointer& other) RAPIDJSON_NOEXCEPT {
                internal::Swap(allocator_, other.allocator_);
                internal::Swap(ownAllocator_, other.ownAllocator_);
                internal::Swap(nameBuffer_, other.nameBuffer_);
                internal::Swap(tokens_, other.tokens_);
                internal::Swap(tokenCount_, other.tokenCount_);
                internal::Swap(parseErrorOffset_, other.parseErrorOffset_);
                internal::Swap(parseErrorCode_, other.parseErrorCode_);
                return *this;
            }
            // 交换两个GenericPointer对象
            friend inline void swap(GenericPointer& a, GenericPointer& b) RAPIDJSON_NOEXCEPT {
                a.Swap(b);
            }
            /**
             * @brief 将一个Token追加到当前GenericPointer的路径中,并返回一个新的GenericPointer对象
             * 
             * @param token 
             * @param allocator 
             * @return GenericPointer 
             */
            GenericPointer Append(const Token& token, Allocator* allocator=nullptr) const {
                GenericPointer r;
                r.allocator_ = allocator;
                Ch* p = r.CopyFromRaw(*this, 1, token.length+1);// 复制当前GenericPointer对象到r,并会为额外的令牌token分配内存,最终返回的是一个指向新分配的名称缓冲区(nameBuffer_)中未被占用的第一个位置(即这个位置会被用来放额外的token)
                std::memcpy(p, token.name, (token.length+1)*sizeof(Ch));
                // 对r对象的tokens_进行追加
                r.tokens_[tokenCount_].name = p;
                r.tokens_[tokenCount_].length = token.length;
                r.tokens_[tokenCount_].index = token.index;
                return r;
            }
            // 接受一个name和length=>一个新的Token,然后将其添加到当前对象的路径中
            GenericPointer Append(const Ch* name, SizeType length, Allocator* allocator=nullptr) const {
                Token token = {name, length, kPointerInvalidIndex};
                return Append(token, length);
            }
            // 允许将一个T*类型的name直接追加到当前对象的路径中
            template<typename T>
            RAPIDJSON_DISABLEIF_RETURN((internal::NotExpr<internal::IsSame<typename internal::RemoveConst<T>::Type, Ch>>), (GenericPointer)) Append(T* name, Allocator* allocator = nullptr) const {
                return Append(name, internal::StrLen(name), allocator);
            }
            // C++11字符串对应的name=>token,追加到当前对象的路径中
            #if RAPIDJSON_HAS_STDSTRING
                GenericPointer Append(const std::basic_string<Ch>& name, Allocator* allocator=nullptr) const {
                    return Append(name.c_str(), static_cast<SizeType>(name.size(), allocator));
                }
            #endif
            /**
             * @brief 将SizeType类型的索引转换为字符串,并将其追加到当前路径中
             * 令牌可以是数字的形式,此时解释为索引
             * @param index 
             * @param allocator 
             * @return GenericPointer 
             */
            GenericPointer Append(SizeType index, Allocator* allocator=nullptr) const {
                char buffer[21];// 存储index转换为字符串的数组
                char* end = sizeof(SizeType)==4?internal::u32toa(index, buffer) : internal::u64toa(index, buffer);// 根据SizeType的字节数来选择32位整数转换为字符串或64位转换为字符串
                SizeType length = static_cast<SizeType>(end-buffer);
                buffer[length] = '\0';// 构建为字符串的形式
                if(sizeof(Ch)==1){// 此时Ch是单字节类型(如char),所以可以直接将buffer中的数据转换为Ch*
                    Token token = {reinterpret_cast<Ch*>(buffer), length, index};
                    return Append(token, allocator);
                }
                else {// 此时Ch是多字节类型(如wchar),所以要先将buffer转换为Ch数组,再Append
                    Ch name[21];
                    for(size_t i=0;i<=length;i++)
                        name[i] = static_cast<Ch>(buffer[i]);
                    Token token = {name, length, index};
                    return Append(token, allocator);
                }
            }
            // 
            GenericPointer Append(const ValueType& token, Allocator* allocator=nullptr) const {
                if(token.IsString())// 追加字符串
                    return Append(token.GetString(), token.GetStringLength(), allocator);
                else {// 追加索引
                    RAPIDJSON_ASSERT(token.IsUint64());
                    RAPIDJSON_ASSERT(token.GetUint64()<=SizeType(~0));
                    return Append(static_cast<SizeType>(token.GetUint64()), allocator);
                }
            }

    };
}

#endif