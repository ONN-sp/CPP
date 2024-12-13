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
            GenericPointer Append(const ValueType& token, Allocator* allocator=nullptr) const {
                if(token.IsString())// 追加字符串
                    return Append(token.GetString(), token.GetStringLength(), allocator);
                else {// 追加索引
                    RAPIDJSON_ASSERT(token.IsUint64());
                    RAPIDJSON_ASSERT(token.GetUint64()<=SizeType(~0));
                    return Append(static_cast<SizeType>(token.GetUint64()), allocator);
                }
            }
            // 检查当前的GenericPointer是否有效
            bool IsValid() const {
                return parseErrorCode_ = kPointerParseErrorNone;
            }
            // 返回解析错误时的偏移量,即错误发生的位置
            size_t GetParseErrorOffset() const {
                return parseErrorOffset_;
            }
            // 返回解析错误时的错误代码
            PointerParseErrorCode GetParseErrorCode() const {
                return parseErrorCode_;
            }
            // 返回GenericPointer对象的内存分配器
            Allocator& GetAllocator() {
                return *allocator_;
            }
            // 返回指向Token数组的指针
            const Token* GetTokens() const {
                return tokens_;
            }
            // 返回Token数组中的元素数量
            size_t GetTokenCount() const {
                return tokenCount_;
            }
            // 重载==
            bool operator==(const GenericPointer& rhs) const {
                if(!IsValid() || !rhs.IsValid() || tokenCount_!=rhs.tokenCount_)
                    return false;
                for(size_t i=0;i<tokenCount_;i++) {
                    if(token_[i].index != rhs.tokens_[i].index ||
                       tokens_[i].length != rhs.tokens_[i].length ||
                       (tokens_[i].length != 0&&std::memcmp(tokens_[i].name, rhs.tokens_[i].name, sizeof(Ch)* tokens_[i].length)!=0))
                        return false;
                }
                return true;
            }
            // 重载!=
            bool operator!=(const GenericPointer& rhs) const {
                return !(*this==rhs);
            }
            // 重载<
            bool operator<(const GenericPointer& rhs) const {
                if(!IsValid())
                    return false;
                if(!rhs.IsValid())// rhs这个Pointer对象无效,那么必然小于当前对象
                    return true;
                if(tokenCount_!=rhs.tokenCount_)// 先比较长度
                    return tokenCount_ < rhs.tokenCount_;
                for(size_t i=0;i<tokenCount_;i++) {
                    if(tokens_[i].index!=rhs.tokens_[i].index)// 再比较index索引
                        return tokens_[i].index < rhs.tokens_[i].index;// 再比较长度
                        if(tokens_[i].length!=rhs.tokens_[i].length)
                            return tokens_[i].length<rhs.tokens_[i].length;
                        if(int cmp=std::memcmp(tokens_[i].name, rhs.tokens_[i].name, sizeof(Ch)*tokens_[i].length))// 最后比较name
                            return cmp<0;
                }
                return false;
            }
            /**
             * @brief 将当前GenericPointer对象的tokens_字符串化为字符串表示方式(即JSON Pointer格式),然后写入输出流os
             * 
             * @tparam OutputStream 
             * @param os 
             * @return true 
             * @return false 
             */
            template<typename OutputStream>
            bool Stringify(OutputStream& os) const {
                return Stringify<false, OutputStream>(os);// 通过模板参数 uriFragment,该方法可以选择输出标准的JSON Pointer格式或URI Fragment格式
            }
            // 将当前GenericPointer对象的tokens_字符串化为URI片段表示方式,然后写入输出流os
            template<typename OutputStream>
            bool StringifyUriFragment(OutputStream& os) const {
                return Stringify<true, OutputStream>(os);
            }
            /**
             * @brief 根据当前对象中的tokens_,动态地在root(一般是一个Document)中创建或查找相应的节点值,如果查找不到,就会在tokens_对应的JSON文档处创建一个空节点(ValueType())
             * 即根据给定的Pointer路径格式查找相应的节点,如果查找到了就直接返回;否则创建一个新的节点,并返回这个新节点
             * 
             * @param root 
             * @param allocator 
             * @param alreadyExist 这是一个表示根据当前Pointer对象的tokens_进行查找节点时,是否能找到的标记.若为true,则表示传入的DOM树(root)能根据tokens_找到最终的节点;否则,就找不到,此时会在DOM树中创建根据tokens_表示的路径处的节点,只是这是一个空节点(即ValueType())
             * @return ValueType& 
             */
            ValueType& Create(ValueType& root, typename ValueType::AllocatorType& allocator, bool* alreadyExist=false) const {
                RAPIDJSON_ASSERT(IsValid());// 确保当前对象有效,因为要访问后续tokens_
                ValueType* v=&root;// 当前根节点,一般是DOM树的根节点
                bool exist = true;// 标志根据当前对象中的tokens_找的节点在当前DOM树中是否存在
                for(const Token* t=tokens_;t!=tokens_+tokenCount_;++t) {// 遍历所有token
                    if(v->IsArray()&&t->name[0]=='-'&&t->length==1) {// 单个负号,定义为数组最后一个元素的下一个元素,即一个新元素,此时相当于标志着需要进行Value::PushBack()
                        v->PushBack(ValueType().Move(), allocator);// 在数组末尾添加一个新的空值
                        v = &((*v)[v->Size()-1]);// 更新v为新添加的这个节点,以便于后续继续处理token
                        exist = false;// 这是创建的,所以之前是不存在的
                    }
                    else {// 处理JSON对象或者数组索引(即不是'-')
                        if(t->index==kPointerInvalidIndex) {// 表示当前路径标记是对象的名称,而不是数组索引
                            if(!v->IsObject())
                                v->SetObject();
                        }
                        else {// 此时当前路径标记表示的是数组
                            if(!v->IsArray()&&!v->IsObject())
                                v->SetArray();
                        }
                        if(v->IsArray()) {// 处理数组索引
                            if(t->index >= v->Size()) {// 需要扩容
                                v->Reserve(t->index+1, allocator);// 扩容
                                while(t->index >= v->Size())// 持续PushBack,直到表示JSON数组的节点v给塞满到t->index+1处,即index前面不能是空的,要一直塞空节点(ValueType()创建一个空节点)
                                    v->PushBack(ValueType().Move(), allocator);
                                exist = false;// 此时是通过扩容新创建的节点,所以exist=false
                            }
                            v = &((*v)[t->index]);// 更新v为当前索引位置的节点
                        }
                        else {// 处理对象成员
                            typename ValueType::MemberIterator m =v->FindMember(GenericValue<EncodingType>(GenericStringRef<Ch>(t->name, t->length)));// 查找是否能在当前v这个JSON对象中找到名称为t->name的成员
                            if(m==v->MemberEnd()) {// 没找到,就要新创建
                                v->AddMember(ValueType(t->name, t->length, allocator).Move(), ValueType().Move(), allocator);// 创建新成员
                                m = v->MemberEnd();
                                v = &(--m)->value;// 指向新创建的这个成员的值,即这个value就是对应的当前token要找的位节点值
                                exist = false;// 新创建的成员节点,所以exist=false
                            }   
                            else
                                v = &m->value;// 可以查找到,那么直接让v指向这个成员的值
                        }
                            
                    }
                }
                if(alreadyExist)// 如果非空,就代表设置了这个参数
                    *alreadyExist = exist;
                return *v;
            }
            // 传入DOM树的上层封装函数
            template<typename stackAllocator>
            ValueType& Create(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, bool* alreadyExist=false) const {
                return Create(document, document.GetAllocator(), alreadyExist);
            }
            /**
             * @brief 根据给定的root(通常就是JSON文档)和根URI解析和合成一个完整的URI地址
             * 从JSON文档中找到的id字段与传入的根URI结合,生成一个新的URI
             * 如果某个token找不到会返回空的UriType(allocator)
             * @param root 
             * @param rootUri 
             * @param unresolvedTokenIndex 定义一个表示无法解析的某个路径字段,即某个token可能DOM树中没有,那么这个参数可以记录是对应的哪个token无法解析
             * @param allocator 
             * @return UriType 
             */
            UriType GetUri(ValueType& root, const UriType& rootUri, size_t* unresolvedTokenIndex=0, Allocator* allocator=nullptr) const {
                static const Ch kIdString[] = {'i', 'd', '\0'};// URI就是找id这个字段
                static const ValueType kIdValue(kIdString, 2);// kIdValue是id字段的字符串值,在后续的操作中用来查找JSON对象中的id键
                UriType base = UriType(rootUri, allocator);// 根URI
                RAPIDJSON_ASSERT(IsValid());
                ValueType* v = &root;// 当前根节点,一般是DOM树的根节点
                for(const Token* t=tokens_;t!=tokens_+tokenCount_;++t) {// 遍历tokens_
                    switch(v->GetType()) {
                        case kObjectType:// 查找id字段对应的uri,只会出现在JSON对象中
                        {
                            typename ValueType::MemberIterator m = v->FindMember(kIdValue);// 查找JSON对象中的id字段.如果找到且id字段的值是字符串类型,则解析该字符串并将其与当前的base URI合成,形成一个新的URI 
                            if(m!=v->MemberEnd()&&(m->value).IsString()) {// 找到了id字段
                                UriType here = UriType(m->value, allocator).Resolve(base, allocator);// Resolve 操作实现了将通过 tokens 找到的 "id" 值加载到 base URI 后面的功能
                                base = here;
                            }
                            m = v->FindMember(GenericValue<EncodingType>(GenericStringRef<Ch>(t->name, t->length)));// 继续沿着当前路径片段t->name对应的字段在DOM树中查找
                            if(m==v->MemberEnd())// 查找不到当前token
                                break;
                            v = &m->value;// 将v更新为当前成员的value,以便在后续的路径解析中使用
                        }
                            continue;
                        case kArrayType:// 数组类型,虽然uri不存在里面,但是要处理表示数组索引的token,以便可以进行后续解析
                            if(t->index==kPointerInvalidIndex || t->index>= v->Size())
                                break;
                            v = &((*v)[t->index]);
                            continue;
                        default:
                            break;
                    }
                    if(unresolvedTokenIndex)// 如果无法解析某个路径片段(即没有找到对应的值),则更新unresolvedTokenIndex,指示哪个路径片段没有被解析成功
                        *unresolvedTokenIndex = static_cast<size_t>(t-tokens_);
                    return UriType(allocator);
                }
                return base;
            }
            UriType GetUri(const ValueType& root, const UriType& rootUri, size_t* unresolvedTokenIndex=nullptr, Allocator* allocator=nullptr) const {
                return GetUri(const_cast<ValueType&>(root), rootUri, unresolvedTokenIndex, allocator);
            }
            /**
             * @brief 根据当前Pointer对象中的tokens_返回要找的节点;找不到
             * 如果某个token找不到会返回nullptr
             * @param root 
             * @param unresolvedTokenIndex 
             * @return ValueType* 
             */
            ValueType* Get(ValueType& root, size_t* unresolvedTokenIndex=nullptr) const {
                RAPIDJSON_ASSERT(IsValid());
                ValueType* v = &root;
                for(const Token* t=tokens_;t!=tokens_+tokenCount_;++t) {// 遍历tokens_
                    switch(v->GetType()) {
                        case kObjectType:
                        {
                            typename ValueType::MemberIterator m = v->FindMember(GenericValue<EncodingType>(GenericStringRef<Ch>(t->name, t->length)));// 在DOM树中查找当前token(t->name)
                            if(m==v->MemberEnd())// 找不到
                                break;
                            v = &m->value;// 找到了
                        }
                        continue;
                    case kArrayType:
                        if(t->index==kPointerInvalidIndex || t->index>=v->Size())// 越界索引,即在当前DOM树中找不到按当前token需要找的节点
                            break;
                        v = &((*v)[t->index]);
                        continue;
                    default:
                        break;
                    }
                    if(unresolvedTokenIndex)// 如果无法解析某个路径片段(即没有找到对应的值),则更新unresolvedTokenIndex,指示哪个路径片段没有被解析成功
                        *unresolvedTokenIndex = static_cast<size_t>(t-tokens_);
                    return nullptr;
                }
                return v;
            }
            const ValueType* Get(const ValueType& root, size_t* unresolvedTokenIndex=nullptr) const {
                return Get(const_cast<ValueType&>(root), unresolvedTokenIndex);
            }
            /**
             * @brief 根据当前Pointer对象中的tokens_返回要找的节点
             * 如果某个token找不到就返回一个默认ValueType对象defaultValue
             * @param root 
             * @param defaultValue 
             * @param allocator 
             * @return ValueType& 
             */
            ValueType& GetWithDefault(ValueType& root, const ValueType& defaultValue, typename ValueType::AllocatorType& allocator) const {
                bool alreadyExist;
                ValueType& v = Create(root, allocator, &alreadyExist);
                return alreadyExist ? v:v.CopyFrom(defaultValue, allocator);
            }
            ValueType& GetWithDefault(ValueType& root, const Ch* defaultValue, typename ValueType::AllocatorType& allocator) const {
                bool alreadyExist;
                ValueType& v = Create(root, allocator, &alreadyExist);
                return alreadyExist ? v:v.SetString(defaultValue, allocator);
            }
            #if RAPIDJSON_HAS_STDSTRING
                ValueType& GetWithDefault(ValueType& root, const std::basic_string<Ch>& defaultValue, typename ValueType::AllocatorType& allocator) const {
                    bool alreadyExist;
                    ValueType& v = Create(root, allocator, &alreadyExist);
                    return alreadyExist ? v : v.SetString(defaultValue, allocator);
                }
            #endif
            template <typename T>
            RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T>>), (ValueType&)) GetWithDefault(ValueType& root, T defaultValue, typename ValueType::AllocatorType& allocator) const {
                return GetWithDefault(root, ValueType(defaultValue).Move(), allocator);
            }
            // 以Document为类型传入
            template<typename stackAllocator>
            ValueType& GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const ValueType& defaultValue) const {
                return GetWithDefault(document, defaultValue, document.GetAllocator());
            }
            template<typename stackAllocator>
            ValueType& GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const Ch* defaultValue) const {
                return GetWithDefault(document, defaultValue, document.GetAllocator());
            }
            #if RAPIDJSON_HAS_STDSTRING
                template<typename stackAllocator>
                ValueType& GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const std::basic_string<Ch>& defaultValue) const {
                    return GetWithDefault(document, defaultValue, document.GetAllocator());
                } 
            #endif
            template<typename T, typename stackAllocator>
            RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T>>), (ValueType&)) GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, T defaultValue) const {
                return GetWithDefault(document, defaultValue, document.GetAllocator());
            }
    };
}

#endif