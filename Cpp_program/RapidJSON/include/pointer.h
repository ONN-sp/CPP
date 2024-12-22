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
                  tokens_(),
                  tokenCount_(),
                  parseErrorOffset_(),
                  parseErrorCode_(kPointerParseErrorNone)
                {
                    *this = rhs;
                }
            GenericPointer(const GenericPointer& rhs, Allocator* allocator)
                : allocator_(allocator),
                  ownAllocator_(),
                  nameBuffer_(),
                  tokens_(),
                  tokenCount_(),
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
                return parseErrorCode_ == kPointerParseErrorNone;
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
                    if(tokens_[i].index != rhs.tokens_[i].index ||
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
            // 将当前GenericPointer对象的tokens_字符串转化为URI片段表示方式,然后写入输出流os
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
            ValueType& Set(ValueType& root, ValueType& value, typename ValueType::AllocatorType& allocator) const {
                return Create(root, allocator) = value;
            }
            ValueType& Set(ValueType& root, const ValueType& value, typename ValueType::AllocatorType& allocator) const {
                return Create(root, allocator).CopyFrom(value, allocator);
            }
            ValueType& Set(ValueType& root, const Ch* value, typename ValueType::AllocatorType& allocator) const {
                return Create(root, allocator) = ValueType(value, allocator).Move();
            }
            // 针对启用了std::string的情况             
            #if RAPIDJSON_HAS_STDSTRING
                ValueType& Set(ValueType& root, const std::basic_string<Ch>& value, typename ValueType::AllocatorType& allocator) const {
                    return Create(root, allocator) = ValueType(value, allocator).Move();
                }  
            #endif
            template<typename T, typename stackAllocator>
            RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T>>), (ValueType&)) Set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, T value) const {
                return Create(document) = value;// 注意:这里其实是用的GenericValue中的赋值运算符的重载(RAPIDJSON_DISABLEIF_RETURN((internal::IsPointer<T>), (GenericValue&)) operator=(T value)).这里不是使用ValueType(value, allocator).Move(),因为GenericValue构造函数没有针对模板参数T的
            }
            template <typename stackAllocator>
            ValueType& Set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, ValueType& value) const {
                return Create(document) = value;
            }
            template <typename stackAllocator>
            ValueType& Set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const ValueType& value) const {
                return Create(document).CopyFrom(value, document.GetAllocator());
            }
            template <typename stackAllocator>
            ValueType& Set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const Ch* value) const {
                return Create(document) = ValueType(value, document.GetAllocator()).Move();
            }
            #if RAPIDJSON_HAS_STDSTRING
                //! Sets a std::basic_string in a document.
                template <typename stackAllocator>
                ValueType& Set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const std::basic_string<Ch>& value) const {
                    return Create(document) = ValueType(value, document.GetAllocator()).Move();
                }
            #endif
            /**
             * @brief 根据当前Pointer对象中的tokens_得到的节点与传入的value进行交换
             * 
             * @param root 
             * @param value 
             * @param allocator 
             * @return ValueType& 
             */
            ValueType& Swap(ValueType& root, ValueType& value, typename ValueType::AllocatorType& allocator) const {
                Create(root, allocator).Swap(value);
            }
            template <typename stackAllocator>
            ValueType& Swap(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, ValueType& value) const {
                return Create(document).Swap(value);
            }
            /**
             * @brief 从传入的root中(一般就是一个Document对象,即DOM树)删除根据当前Pointer对象中的tokens_解析到的节点
             * 
             * @param root 
             * @return true 
             * @return false 
             */
            bool Erase(ValueType& root) const {
                RAPIDJSON_ASSERT(IsValid());
                if(tokenCount_==0)
                    return false;
                ValueType* v = &root;
                const Token* last = tokens_+(tokenCount_-1);// tokens_的尾地址
                for(const Token* t=tokens_;t!=last;++t) {// 遍历访问每一个token
                    switch(v->GetType()) {
                        case kObjectType:
                            {
                                typename ValueType::MemberIterator m = v->FindMember(GenericValue<EncodingType>(GenericStringRef<Ch>(t->name, t->length)));
                                if(m==v->MemberEnd())
                                    return false;
                                v = &m->value;
                            }
                            break;
                        case kArrayType:
                            if(t->index==kPointerInvalidIndex||t->index>=v->Size())// 按当前token的索引t->index找不到
                                return false;
                            v = &((*v)[t->index]);
                            break;
                        default:
                            return false;
                    }
                }
                // 按照tokens_路径最终找到的节点就是此时last->name(即按tokens_的每个路径片段逐层找到最终节点)对应的节点或v->Begin()+last
                switch(v->GetType()) {
                    case kObjectType:
                        return v->EraseMember(GenericStringRef<Ch>(last->name, last->length));
                    case kArrayType:
                        if(last->index==kPointerInvalidIndex || last->index>=v->Size())
                            return false;
                        v->Erase(v->Begin()+last->index);
                        return true;
                    default:
                        return false;
                }
            }
        private:
            /**
             * @brief 将一个给定的Pointer对象rhs中的所有Token和相应的名称数据复制到当前对象中
             * 它会分配内存,并将所有相关的数据从rhs复制到当前对象
             * 返回新分配的nameBuffer_的结束位置,即下一个可用的(没被占用的)内存地址
             * @param rhs 
             * @param extraToken 
             * @param extraNameBufferSize 
             * @return Ch* 
             */
            Ch* CopyFromRaw(const GenericPointer& rhs, size_t extraToken=0, size_t extraNameBufferSize=0) {
                if(!allocator_)// 如果当前对象没有分配allocator,则为其分配一个新的 allocator
                    ownAllocator_ = allocator_ = RAPIDJSON_NEW(Allocator)();
                // 计算nameBuffer的大小,nameBuffer用来存储所有token的名称(token.name)
                size_t nameBufferSize = rhs.tokenCount_;// 每个token有一个'\0'终结符,所以以rhs.tokenCount_为起点开始加
                for(Token* t=rhs.tokens_;t!=rhs.tokens_+rhs.tokenCount_;++t)
                    nameBufferSize += t->length;// 累加每个token的名称name的长度
                tokenCount_ = rhs.tokenCount_+extraToken;// 为tokenCount_增加额外的tokens(由extraToken指定)
                // 分配内存:首先为tokens分配内存,再为nameBuffer分配内存(包含extraNameBufferSize指定的额外空间)
                tokens_ = static_cast<Token*>(allocator_->Malloc(tokenCount_*sizeof(Token)+(nameBufferSize+extraNameBufferSize)*sizeof(Ch)));
                nameBuffer_ = reinterpret_cast<Ch*>(tokens_+tokenCount_);// nameBuffer是存储所有token名称(token->name)的缓冲区,分配空间紧接在tokens_之后
                if(rhs.tokenCount_>0)// 复制rhs中的tokens_数据
                    std::memcpy(tokens_, rhs,tokens_, rhs.tokenCount_*sizeof(Token));
                if(nameBufferSize > 0)// 复制rhs中的nameBuffer(即rhs中的tokens_的名称(token.name))
                    std::memcpy(nameBuffer_, rhs.nameBuffer_, nameBufferSize*sizeof(Ch));
                for(size_t i=0;i<rhs.tokenCount_;++i) {// 更新当前对象的每一个token.name
                    std::ptrdiff_t name_offset = rhs.tokens_[i].name-rhs.nameBuffer_;
                    tokens_[i].name = nameBuffer_+name_offset;
                }
                return nameBuffer_+nameBufferSize;// 返回新分配的nameBuffer的结束位置,即返回的指针是nameBuffer的最后位置
            }  
            // 确定字符c是否属于URI中不需要编码的安全字符集('0'-'9'、'a'-'z'、'A'-'Z'、'-'、'.'、'_'、'~'为安全字符集).安全字符集不用百分号编码(%+ASCII码值)
            // 此函数在涉及到URI片段表示时可能使用(如:字符串化tokens_为uri片段表示方式时,即Stringify())
            bool NeedPercentEncode(Ch c) const {
                return !((c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='-'||c=='.'||c=='_'||c=='~');
            }
            /**
             * @brief 解析一个源字符串source,并将结果保存在tokens_和nameBuffer_中
             * source可能是普通的字符串表示,也可能是URI片段表示方式
             * @param source 
             * @param length 
             */
            void Parse(const Ch* source, size_t length) {
                RAPIDJSON_ASSERT(source!=nullptr);
                RAPIDJSON_ASSERT(nameBuffer_==0);// 使用Parse()的构造函数,那么此时是要定义nameBuffer_的,所以这里要确保要定义nameBuffer_
                RAPIDJSON_ASSERT(tokens_==0);
                if(!allocator_)// 如果当前对象没有分配allocator,则为其分配一个新的 allocator
                    ownAllocator_ = allocator_ = RAPIDJSON_NEW(Allocator)();
                tokenCount_ = 0;
                for(const Ch* s=source;s!=source+length;++s)
                    if(*s=='/')// 统计源字符串source中'/'字符的个数,即token的数量,将其作为tokenCount_
                        tokenCount_++;
                    Token* token = tokens_ = static_cast<Token*>(allocator_->Malloc(tokenCount_*sizeof(Token)+length*sizeof(Ch)));// 分配内存:首先为tokens分配内存,再为nameBuffer_分配内存
                    Ch* name = nameBuffer_ = reinterpret_cast<Ch*>(tokens_+tokenCount_);// nameBuffer_是存储所有token名称(token->name)的缓冲区,分配空间紧接在tokens_之后
                    size_t i = 0;
                    bool uriFragment = false;
                    if(source[i]=='#') {// 以#为开头,就表示传入的这个source是URI片段表示形式的字符串
                        uriFragment = true;
                        i++;// 跳过'#'字符
                    }
                    if(i!=length&&source[i]!='/') {// 对于字符串表示方式,它的第一个字符应该是'/';对于URI片段表示方式,'#'后的第一个字符也应该是'/'
                        parseErrorCode_ = kPointerParseErrorTokenMustBeginWithSolidus;
                        goto error;// 跳转到error处理
                    }
                    while(i<length) {// 进行'/'后续的处理
                        RAPIDJSON_ASSERT(source[i]=='/');
                        i++;
                        token->name = name;// 将token的name指向当前分段的开始位置(nameBuffer_)
                        bool isNumber = true;// 对于'%'后的一般是数字
                        while(i<length&&source[i]!='/') {
                            Ch c = source[i];
                            if(uriFragment) {// 为真则表示路径是URI片段,需要对某些字符进行百分号编码处理
                                if(c=='%') {// 此时需要解码百分号编码
                                    PercentDecodeStream is(&source[i], source+length);
                                    GenericInsituStringStream<EncodingType> os(name);// 解码成功后会把解码后的字符写入到name指向的缓冲区
                                    Ch* begin = os.PutBegin();
                                    if(!Transcoder<UTF8<>, EncodingType>().Validate(is, os) || !is.IsValid()) {// 验证解码是否成功
                                        parseErrorCode_ = kPointerParseErrorInvalidPercentEncoding;
                                        goto error;
                                    }
                                    size_t len = os.PutEnd(begin);// 解码后的字符的长度
                                    i += is.Tell()-1;// 跳过已解码的字符
                                    if(len==1)// 如果解码后的字符只有一个字符,则直接用解码后的字符*name赋给c
                                        c = *name;// 赋给c目的是便于后续的*name++=c的操作的统一
                                    else {// 解码后有多个字符
                                        name += len;// 将指针name移动len个位置,跳过已解码的字符(其实在解码的时候name指针指向的内存已经赋值解码后的字符了)
                                        isNumber = false;
                                        i++;
                                        continue;
                                    }
                                }
                                else if(NeedPercentEncode(c)) {// 如果c不是'%',且c是需要百分号编码的字符(即非安全字符集的字符)此时会报错,因为在一个合合法的URI片段中,不可能还会有未进行百分号编码的非安全字符
                                    parseErrorCode_ = kPointerParseErrorCharacterMustPercentEncode;
                                    goto error;
                                }
                            } 
                            i++;
                            // 处理转义字符,即:"~0" -> '~', "~1" -> '/'
                            if(c=='~') {
                                if(i<length) {
                                    c = source[i];
                                    if(c=='0')
                                        c = '~';// '~0'='~'
                                    else if(c=='1')
                                        c = '/';// '~1'=>'/'
                                    else {
                                        parseErrorCode_ = kPointerParseErrorInvalidEscape;
                                        goto error;
                                    }
                                    i++;
                                }
                                else {
                                    parseErrorCode_ = kPointerParseErrorInvalidEscape;
                                    goto error;
                                }
                            }
                            if(c<'0'||c>'9')// 检查当前字符是否是数字
                                isNumber = false;
                            *name++ = c;
                        }
                        token->length = static_cast<SizeType>(name-token->name);// 计算当前解析这个token的长度
                        if(token->length==0)
                            isNumber = false;
                        *name++ = '\0';// 添加终止符
                        if(isNumber&&token->length>1&&token->name[0]=='0')// 此时这个token是一个有着前导零的路径片段表示,表示它不是数字,因为数字不会有前导零
                            isNumber = false;
                        SizeType n = 0;
                        if(isNumber) {// 将表示数字的token转换为数值类型SizeType
                            for(size_t j=0;j<token->length;++j) {// 按位转换为对应数值
                                SizeType m = n*10+static_cast<SizeType>(token->name[j]-'0');
                                if(m < n) {// 转换过程中发生溢出,则终止转换
                                    isNumber = false;
                                    break;
                                }
                                n = m;
                            }
                        }
                        token->index = isNumber?n:kPointerInvalidIndex;// 保存当前解析后写入的token的索引
                        token++;// 解析下一个token
                    }
                    RAPIDJSON_ASSERT(name<=nameBuffer_+length);
                    parseErrorCode_ = kPointerParseErrorNone;// 解析没错的话,就是kPointerParseErrorNone
                    return;
                error:// Parse()解析过程中发生错误,会跳转到当前这个error标签,并执行下面的代码
                    Allocator::Free(tokens_);
                    nameBuffer_ = 0;
                    tokens_ = 0;
                    tokenCount_ = 0;
                    parseErrorOffset_ = i;
                    return;
            }
            template<bool uriFragment, typename OutputStream>
            bool Stringify(OutputStream& os) const {
                RAPIDJSON_ASSERT(IsValid());
                if(uriFragment)
                    os.Put('#');
                for(Token* t=tokens_;t!=tokens_+tokenCount_;++t) {
                    os.Put('/');
                    for(size_t j=0;j<t->length;++j) {
                        Ch c = t->name[j];
                        if(c=='~') {// 编码转义字符'~'
                            os.Put('~');
                            os.Put('0');
                        }
                        else if(c=='/') {// 编码转义字符'/'
                            os.Put('~');
                            os.Put('1');
                        }
                        else if(uriFragment&&NeedPercentEncode(c)) {// 编码URI中的非安全字符
                            GenericStringStream<typename ValueType::EncodingType> source(&t->name[j]);
                            PercentEncodeStream<OutputStream> target(os);
                            if(!Transcoder<EncodingType, UTF8<>>().Validate(source, target))
                                return false;
                            j += source.Tell()-1;
                        }
                        else// 其余情况,直接写入
                            os.Put(c);
                    }
                }
                return true;
            }
            /**
             * @brief 解码URL编码得到的字符串序列
             * %XY=>对应字符
             * 
             */
            class PercentDecodeStream {
                public:
                    typedef typename ValueType::Ch Ch;
                    PercentDecodeStream(const Ch* source, const Ch* end)
                        : src_(source),
                          end_(end),
                          valid_(true)
                        {}
                    Ch Take() {
                        if(*src_!='%' || src_+3>end_) {// %XY表示至少要有三个字符
                            valid_ = false;
                            return 0;
                        }
                        src_++;
                        Ch c = 0;
                        for(int j=0;j<2;++j) {// 解析%后的两个十六进制
                            c = static_cast<Ch>(c<<4);
                            Ch h = *src_;
                            if (h >= '0' && h <= '9') 
                                c = static_cast<Ch>(c + h - '0');// 字符为'0'-'9'
                            else if (h >= 'A' && h <= 'F')// 如果是'A'-'F'或'a'-'f',则将其转换为相应的数字值(如'A'对应10,'F'对应15)
                                c = static_cast<Ch>(c + h - 'A' + 10);
                            else if (h >= 'a' && h <= 'f') 
                                c = static_cast<Ch>(c + h - 'a' + 10);
                            else {
                                valid_ = false;
                                return 0;
                            }
                            src_++;
                        }
                        return c;
                    }
                    size_t Tell() const {
                        return static_cast<size_t>(src_-head_);
                    }
                    bool IsValid() const {
                        return valid_;
                    }
                private:
                    const Ch* src_;
                    const Ch* head_;
                    const Ch* end_;
                    bool valid_;
            };
            /**
             * @brief 将传入的字符流中的字符,按照URL编码方式转换成%XY格式的编码,其中X和Y是十六进制数
             * 
             * @tparam OutputStream 
             */
            template<typename OutputStream>
            class PercentEncodeStream {
                public:
                    PercentEncodeStream(OutputStream& os)  : os_(os) {}
                    void Put(char c) {
                        unsigned char u = static_cast<unsigned char>(c);
                        static const char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                        os_.Put('%');// 写入'%'
                        os_.Put(static_cast<typename OutputStream::Ch>(hexDigits[u>>4]));// 得到高四位并写入
                        os_.Put(static_cast<typename OutputStream::Ch>(hexDigits[u&15]));// 得到低四位并写入
                    }
                private:
                    OutputStream& os_;
            };
            Allocator* allocator_;// 当前分配器.它要么由用户提供,要么等于ownAllocator_
            Allocator* ownAllocator_;// 当前对象所指向的分配器
            Ch* nameBuffer_;// 一个缓冲区,包含tokens_的所有名称(name)
            Token* tokens_;// token数组
            size_t tokenCount_;// tokens_中的token数目
            size_t parseErrorOffset_;// 解析失败时的代码单位偏移量
            PointerParseErrorCode parseErrorCode_;// 解析失败的错误码
    };
    typedef GenericPointer<Value> Pointer;
    /**
     * @brief 通过传入一个Pointer对象和root,然后在root中按传入的Pointer对象的tokens_对root进行Create()(查找或创建)
     * 
     * @tparam T 
     * @param root 
     * @param pointer 
     * @param a 
     * @return T::ValueType& 
     */
    template<typename T>
    typename T::ValueType& CreateValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, typename T::AllocatorType& a) {
        return pointer.Create(root, a);
    }
    template<typename T, typename CharType, size_t N>
    typename T::ValueType& CreateValueByPointer(T& root, const CharType(&source)[N], typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Create(root, a);
    }
    template<typename DocumentType>
    typename DocumentType::ValueType& CreateValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer) {
        return pointer.Create(document);
    }
    template<typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& CreateValueByPointer(DocumentType& document, const CharType(&source)[N]) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Create(document);
    }
    /**
     * @brief 返回通过传入的Pointer对象的tokens_在root中获取相应的路径查找到的节点
     * 
     * @tparam T 
     * @param root 
     * @param pointer 
     * @param unresolvedTokenIndex 
     * @return const T::ValueType* 
     */
    template<typename T>
    const typename T::ValueType* GetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, size_t* unresolvedTokenIndex=0) {
        return pointer.Get(root, unresolvedTokenIndex);
    }
    template <typename T>
    const typename T::ValueType* GetValueByPointer(const T& root, const GenericPointer<typename T::ValueType>& pointer, size_t* unresolvedTokenIndex = 0) {
        return pointer.Get(root, unresolvedTokenIndex);
    }
    template<typename T, typename CharType, size_t N>
    const typename T::ValueType* GetValueByPointer(T& root, const CharType(&source)[N], size_t* unresolvedTokenIndex=0) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Get(root, unresolvedTokenIndex);
    }
    template <typename T, typename CharType, size_t N>
    const typename T::ValueType* GetValueByPointer(const T& root, const CharType(&source)[N], size_t* unresolvedTokenIndex = 0) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Get(root, unresolvedTokenIndex);
    }
    /**
     * @brief 返回通过传入的Pointer对象的tokens_在root中获取相应的路径查找到的节点
     * 如果某个token找不到就返回一个默认ValueType对象defaultValue
     * 
     * @tparam T 
     * @param root 
     * @param pointer 
     * @param defaultValue 
     * @param a 
     * @return T::ValueType& 
     */
    template<typename T>
    typename T::ValueType& GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::ValueType& defaultValue, typename T::AllocatorType& a) {
        return pointer.GetWithDefault(root, defaultValue, a);
    }
    template <typename T>
    typename T::ValueType& GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::Ch* defaultValue, typename T::AllocatorType& a) {
        return pointer.GetWithDefault(root, defaultValue, a);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename T>
    typename T::ValueType& GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, const std::basic_string<typename T::Ch>& defaultValue, typename T::AllocatorType& a) {
        return pointer.GetWithDefault(root, defaultValue, a);
    }
    #endif
    template <typename T, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
    GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, T2 defaultValue, typename T::AllocatorType& a) {
        return pointer.GetWithDefault(root, defaultValue, a);
    }
    template <typename T, typename CharType, size_t N>
    typename T::ValueType& GetValueByPointerWithDefault(T& root, const CharType(&source)[N], const typename T::ValueType& defaultValue, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
    }
    template <typename T, typename CharType, size_t N>
    typename T::ValueType& GetValueByPointerWithDefault(T& root, const CharType(&source)[N], const typename T::Ch* defaultValue, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename T, typename CharType, size_t N>
    typename T::ValueType& GetValueByPointerWithDefault(T& root, const CharType(&source)[N], const std::basic_string<typename T::Ch>& defaultValue, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
    }
    #endif
    template <typename T, typename CharType, size_t N, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
    GetValueByPointerWithDefault(T& root, const CharType(&source)[N], T2 defaultValue, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
    }
    // 无传入的内存分配器参数   Document对象有成员allocator_,所以使用函数以DocumentType为参数时就不用传入AllocatorType了
    template<typename DocumentType>
    typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::ValueType& defaultValue) {
        return pointer.GetWithDefault(document, defaultValue);
    }
    template <typename DocumentType>
    typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::Ch* defaultValue) {
        return pointer.GetWithDefault(document, defaultValue);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename DocumentType>
    typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const std::basic_string<typename DocumentType::Ch>& defaultValue) {
        return pointer.GetWithDefault(document, defaultValue);
    }
    #endif
    template <typename DocumentType, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
    GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, T2 defaultValue) {
        return pointer.GetWithDefault(document, defaultValue);
    }
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], const typename DocumentType::ValueType& defaultValue) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
    }
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], const typename DocumentType::Ch* defaultValue) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], const std::basic_string<typename DocumentType::Ch>& defaultValue) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
    }
    #endif
    template <typename DocumentType, typename CharType, size_t N, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
    GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], T2 defaultValue) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
    }
    /**
     * @brief 设置通过传入的Pointer对象的tokens_在root中获取相应的路径查找到的节点为传入的value
     * 
     * @tparam T 
     * @param root 
     * @param pointer 
     * @param value 
     * @param a 
     * @return T::ValueType& 
     */
    template<typename T>
    typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, typename T::ValueType& value, typename T::AllocatorType& a) {
        return pointer.Set(root, value, a);
    }
    template <typename T>
    typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::ValueType& value, typename T::AllocatorType& a) {
        return pointer.Set(root, value, a);
    }
    template <typename T>
    typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::Ch* value, typename T::AllocatorType& a) {
        return pointer.Set(root, value, a);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename T>
    typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, const std::basic_string<typename T::Ch>& value, typename T::AllocatorType& a) {
        return pointer.Set(root, value, a);
    }
    #endif
    template <typename T, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
    SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, T2 value, typename T::AllocatorType& a) {
        return pointer.Set(root, value, a);
    }
    template <typename T, typename CharType, size_t N>
    typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], typename T::ValueType& value, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Set(root, value, a);
    }
    template <typename T, typename CharType, size_t N>
    typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], const typename T::ValueType& value, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Set(root, value, a);
    }
    template <typename T, typename CharType, size_t N>
    typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], const typename T::Ch* value, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Set(root, value, a);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename T, typename CharType, size_t N>
    typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], const std::basic_string<typename T::Ch>& value, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Set(root, value, a);
    }
    #endif
    template <typename T, typename CharType, size_t N, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
    SetValueByPointer(T& root, const CharType(&source)[N], T2 value, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N - 1).Set(root, value, a);
    }
    // 无传入的内存分配器参数
    template <typename DocumentType>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, typename DocumentType::ValueType& value) {
        return pointer.Set(document, value);
    }
    template <typename DocumentType>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::ValueType& value) {
        return pointer.Set(document, value);
    }
    template <typename DocumentType>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::Ch* value) {
        return pointer.Set(document, value);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename DocumentType>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const std::basic_string<typename DocumentType::Ch>& value) {
        return pointer.Set(document, value);
    }
    #endif
    template <typename DocumentType, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
    SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, T2 value) {
        return pointer.Set(document, value);
    }
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], typename DocumentType::ValueType& value) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Set(document, value);
    }
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], const typename DocumentType::ValueType& value) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Set(document, value);
    }
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], const typename DocumentType::Ch* value) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Set(document, value);
    }
    #if RAPIDJSON_HAS_STDSTRING
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], const std::basic_string<typename DocumentType::Ch>& value) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Set(document, value);
    }
    #endif
    template <typename DocumentType, typename CharType, size_t N, typename T2>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
    SetValueByPointer(DocumentType& document, const CharType(&source)[N], T2 value) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Set(document, value);
    }
    /**
     * @brief 对传入的root(一般会是一个Document对象)按照当前的Pointer对象的tokens_找到的节点和传入的value节点进行交换
     * 
     * @tparam T 
     * @param root 
     * @param pointer 
     * @param value 
     * @param a 
     * @return T::ValueType& 
     */
    template<typename T>
    typename T::ValueType& SwapValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, typename T::ValueType& value, typename T::AllocatorType& a) {
        return pointer.Swap(root, value, a);
    }
    template<typename T, typename CharType, size_t N>
    typename T::ValueType& SwapValueByPointer(T& root, const CharType(&source)[N], typename T::ValueType& value, typename T::AllocatorType& a) {
        return GenericPointer<typename T::ValueType>(source, N-1).Swap(root, value, a);
    }
    template<typename DocumentType>
    typename DocumentType::ValueType& SwapValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, typename DocumentType::ValueType& value) {
        return pointer.Swap(document, value);
    }
    template <typename DocumentType, typename CharType, size_t N>
    typename DocumentType::ValueType& SwapValueByPointer(DocumentType& document, const CharType(&source)[N], typename DocumentType::ValueType& value) {
        return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Swap(document, value);
    }
    /**
     * @brief 从传入的root中(一般就是一个Document对象,即DOM树)删除根据当前传入的Pointer对象中的tokens_解析到的节点
     * 
     * @tparam T 
     * @param root 
     * @param pointer 
     * @return true 
     * @return false 
     */
    template<typename T>
    bool EraseValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer) {
        return pointer.Erase(root);
    }
    template<typename T, typename CharType, size_t N>
    bool EraseValueByPointer(T& root, const CharType(&source)[N]) {
        return GenericPointer<typename T::ValueType>(source, N-1).Erase(root);
    }
}
#endif