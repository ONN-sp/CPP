#ifndef RAPIDJSON_URI_H_
#define RAPIDJSON_URI_H_

#include "internal/strfunc.h"

namespace RAPIDJSON {
    /**
     * @brief 定义一个用于表示URI的模板类,它的目的是将URI字符串拆解成多个部分(scheme、host、path等),并提供相关的接口
     * 
     * @tparam ValueType 
     * @tparam Allocator 
     */
    template<typename ValueType, typename Allocator=CrtAllocator>
    class GenericUri {
        public:
            typedef typename ValueType::Ch Ch;
            #if RAPIDJSON_HAS_STDSTRING
                typedef std::basic_string<Ch> String;
            #endif
            // 默认构造函数
            GenericUri(Allocator* allocator=nullptr) 
                : uri_(),
                  base_(),
                  scheme_(),
                  auth_(),
                  path_(),
                  query_(),
                  frag_(),
                  allocator(allocator),
                  ownAllocator_()
                {}
            // 
            GenericUri(const Ch* uri, SizeType len, Allocator* allocator=nullptr) 
                : uri_(),
                  base_(),
                  scheme_(),
                  auth_(),
                  path_(),
                  query_(),
                  frag_(),
                  allocator(allocator),
                  ownAllocator_()
                {
                    Parse(uri, len);// 将传入的uri字符串的各个部分解析到URI对象中表示各个部分的内存中
                }
            GenericUri(const Ch* uri, Allocator* allocator=nullptr) 
                : uri_(),
                  base_(),
                  scheme_(),
                  auth_(),
                  path_(),
                  query_(),
                  frag_(),
                  allocator(allocator),
                  ownAllocator_()
                {
                    Parse(uri, internal::StrLen<Ch>(uri));
                }
            template<typename T>
            GenericUri(const T& uri, Allocator* allocator=nullptr)
                : uri_(),
                  base_(),
                  scheme_(),
                  auth_(),
                  path_(),
                  query_(),
                  frag_(),
                  allocator(allocator),
                  ownAllocator_()
                {
                    const Ch* u = uri.template Get<const Ch*>();
                    Parse(u, internal::StrLen<Ch>(u));
                }
            #if RAPIDJSON_HAS_STDSTRING
                GenericUri(const String& uri, Allocator* allocator=nullptr) 
                    : uri_(),
                      base_(),
                      scheme_(),
                      auth_(),
                      path_(),
                      query_(),
                      frag_(),
                      allocator(allocator),
                      ownAllocator_()
                    {
                        Parse(uri.c_str(), internal::StrLen<Ch>(uri.c_str()));
                    }
            #endif
            GenericUri(const GenericUri& rhs) 
                : uri_(),
                  base_(),
                  scheme_(),
                  auth_(),
                  path_(),
                  query_(),
                  frag_(),
                  allocator(),
                  ownAllocator_()
                {
                    *this = rhs;
                }
            GenericUri(const GenericUri& rhs, Allocator* allocator) 
                : uri_(),
                  base_(),
                  scheme_(),
                  auth_(),
                  path_(),
                  query_(),
                  frag_(),
                  allocator(allocator),
                  ownAllocator_()
                {
                    *this = rhs;
                }
            // 在对象销毁时,释放内存资源
            ~GenericUri() {
                Free();
                RAPIDJSON_DELETE(ownAllocator_);
            }
            GenericUri& operator=(const GenericUri& rhs) {
                if(this!=&rhs) {// 不是自赋值
                    Free();
                    Allocate(rhs.GetStringLength());
                    auth_ = CopyPart(scheme_, rhs.scheme_, rhs.GetSchemeStringLength());// CopyPart()会返回复制后的内存的下一个位置
                    path_ = CopyPart(auth_, rhs.auth_, rhs.GetAuthStringLength());
                    query_ = CopyPart(path_, rhs.path_, rhs.GetPathStringLength());
                    frag_ = CopyPart(query_, rhs.query_, rhs.GetQueryStringLength());
                    base_ = CopyPart(frag_, rhs.frag_, rhs.GetFragStringLength());
                    uri_ = CopyPart(base_, rhs.base_, rhs.GetBaseStringLength());
                    CopyPart(uri_, rhs.uri_, rhs.GetStringLength());
                }
                return *this;
            }
            /**
              * @brief 将当前的URI对象的URI字符串存储到T类型的对象uri中
             * 
             * @tparam T 
             * @param uri 
             * @param allocator 
             */
            template<typename T>
            void Get(T& uri, Allocator& allocator) {
                uri.template Set<const Ch*>(this->GetString(), allocator);
            }
            // 返回当前URI对象的各个uri部分字符串和其长度
            const Ch* GetString() const { return  uri_;}
            SizeType GetStringLength() const { return uri_==0?0:internal::StrLen<Ch>(uri_);}
            const Ch* GetBaseString() const { return base_;}
            SizeType GetBaseStringLength() const { return base_==0?0::internal::StrLen<Ch>(base_);}
            const Ch* GetSchemeString() const { return scheme_; }
            SizeType GetSchemeStringLength() const { return scheme_ == 0 ? 0 : internal::StrLen<Ch>(scheme_);}
            const Ch* GetAuthString() const { return auth_; }
            SizeType GetAuthStringLength() const { return auth_ == 0 ? 0 : internal::StrLen<Ch>(auth_);}
            const Ch* GetPathString() const { return path_; }
            SizeType GetPathStringLength() const { return path_ == 0 ? 0 : internal::StrLen<Ch>(path_);}
            const Ch* GetQueryString() const { return query_; }
            SizeType GetQueryStringLength() const { return query_ == 0 ? 0 : internal::StrLen<Ch>(query_);}
            const Ch* GetFragString() const { return frag_; }
            SizeType GetFragStringLength() const { return frag_ == 0 ? 0 : internal::StrLen<Ch>(frag_);}
            #if RAPIDJSON_HAS_STDSTRING
                static String Get(const GenericUri& uri) {
                    return  String(uri.GetString(), uri.GetStringLength();)
                }
                static String GetBase(const GenericUri& uri) {
                    return String(uri.GetBaseString(), uri.GetBaseStringLength());
                }
                static String GetScheme(const GenericUri& uri) {
                    return String(uri.GetSchemeString(), uri.GetSchemeStringLength());
                }
                static String GetAuth(const GenericUri& uri) {
                    return String(uri.GetAuthString(), uri.GetAuthStringLength());
                }
                static String GetPath(const GenericUri& uri) {
                    return String(uri.GetPathString(), uri.GetPathStringLength());
                }
                static String GetQuery(const GenericUri& uri) {
                    return String(uri.GetQueryString(), uri.GetQueryStringLength());
                }
                static String GetFrag(const GenericUri& uri) {
                    return String(uri.GetFragString(), uri.GetFragStringLength());
                }
            #endif
            // 比较两个URI对象是否相等
            bool operator==(const GenericUri& rhs) const {
                return Match(rhs, true);
            }
            // 比较两个URI对象是否不相等
            bool operator!=(const GenericUri& rhs) const {
                return !Match(rhs, true);
            }
            /**
             * @brief 用于比较两个URI是否匹配
             * full为true,则比较整个URI字符串uri_;full位false,则只比较base_部分
             * @param uri 
             * @param full 
             * @return true 
             * @return false 
             */
            bool Match(const GenericUri& uri, bool full=true) const {
                Ch* s1;
                Ch* s2;
                if(full) {// 为true比较uri_
                    s1 = uri_;
                    s2 = uri.uri_;
                }
                else {// 为false比较base_
                    s1 = base_;
                    s2 = uri.base_;
                }
                if(s1==s2)
                    return  true;
                if(s1==0 || s2==0)
                    return false;
                return internal::StrCmp<Ch>(s1, s2)==0;
            }
            /**
             * @brief 解析一个相对URI(当前URI对象)相对于一个基准URI(baseuri)的绝对URI.它会根据URI组成部分(shceme、auth、path、query、frag)来拼接、修改并返回解析后的URI
             * 
             * @param baseuri 
             * @param allocator 
             * @return GenericUri 
             */
            GenericUri Resolve(const GenericUri& baseuri, Allocator* allocator=0) {
                GenericUri resuri;// 存储拼接后的URI
                resuri.allocator_ = allocator;
                resuri.Allocate(GetStringLength()+baseuri.GetStringLength()+1);// +1是为了可能在拼接时连接需要所用的'/'
                if(!(GetSchemeStringLength()==0)) {// 如果当前URI有scheme,则直接使用当前URI的各个部分(scheme、auth、path、query和frag)
                    resuri.auth_ = CopyPart(resuri.scheme_, scheme_, GetSchemeStringLength());
                    resuri.path_ = CopyPart(resuri.auth_, auth_, GetAuthStringLength());
                    resuri.query_ = CopyPart(resuri.path_, path_, GetPathStringLength());
                    resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
                    resuri.RemoveDotSegments();// 处理路径中'.'和'..'符合,即'.'表示当前节点,'..'表示退回一层的节点
                }
                else {// 如果当前URI没有scheme,则使用baseuri的scheme
                    resuri.auth_ = CopyPart(resuri.scheme_, baseuri.scheme_, baseuri.GetSchemeStringLength());
                    if(!(GetAuthStringLength()==0)) {// 如果当前URI有auth部分,则使用当前URI的auth、path、query和frag部分
                        resuri.path_ = CopyPart(resuri.auth_, auth_, GetAuthStringLength());
                        resuri.query_ = CopyPart(resuri.path_, path_, GetPathStringLength());
                        resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
                        resuri.RemoveDotSegments();
                    }
                    else {// 如果进一步当前URI没有auth,则使用baseuri的auth
                        resuri.path_ = CopyPart(resuri.auth_, baseuri.auth_, baseuri.GetAuthStringLength());
                        if(GetPathStringLength()=0) {// 如果当前URI没有path部分,则使用baseuri的path
                            resuri.query_ = CopyPart(resuri.path_, baseuri.path_, baseuri.GetPathStringLength());
                            if(GetQueryStringLength()==0)// 如果当前URI没有query部分,则使用baseuri的query
                                resuri.frag_ = CopyPart(resuri.query_, baseuri.query_, baseuri.GetQueryStringLength());
                            else// 如果当前URI有query部分,则使用当前URI对象的query
                                resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
                        }
                        else {// 如果当前URI有path部分,则使用当前URI对象的path.分为绝对路径和相对路径考虑
                            if(path_[0]=='/') {// 当前URI对象的uri字符串表示的是绝对路径
                                resuri.query_ = CopyPart(resuri.path_, path_, GetPathStringLength());// 绝对路径则直接使用这个路径
                                resuri.RemoveDotSegments();
                            }
                            else {// 对于相对路径,首先确baseuri的路径的最后一个'/'位置,然后将当前路径附加到这个位置后面
                                size_t pos = 0;
                                if(!(baseuri.GetAuthStringLength()==0) && baseuri.GetPathStringLength()==0) {// baseuri的路径path_为空,则此时在resuri中的path部分加一个'/'符号作为起始连接符,以此连接当前URI对象的path部分
                                    resuri.path_[pos] = '/';
                                    pos++;
                                }
                                size_t lastslashpos = baseuri.GetPathStringLength();
                                while(lastslashpos > 0) {
                                    if(baseuri.path_[lastslashpos-1]=='/')// 返回baseuri中倒数第一个'/'的位置,便于将当前URI对象的path部分加到这后面
                                        break;
                                    lastslashpos--;
                                }
                                std::memcpy(&resuri.path_[pos], baseuri.path_, lastslashpos*sizeof(Ch));
                                pos += lastslashpos;// 跳过baseuri的path部分
                                resuri.query_ = CopyPart(&resuri.path_[pos], path_, GetPathStringLength());// 将当前URI对象的path部分拼接在后面
                                resuri.RemoveDotSegments();
                            }
                            resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
                        }
                    }
                    resuri.base_ = CopyPart(resuri.frag_, frag_, GetFragStringLength());// 处理fragment部分.base_为不包含fragment的uri_部分
                    resuri.SetBase();// 设置resuri的base_部分
                    resuri.uri_ = resuri.base_ + resuri.GetBaseStringLength()+1;// 
                    resuri.SetUri();// 设置resuri这个URI对象的uri_部分
                    return resuri;
                }
                Allocator& GetAllocator() { return *allocator; }
            private:
                /**
                 * @brief 负责为一个URI对象的各个部分(scheme_,auth_,query_,frag_,uri_)分配内存
                 * 
                 * @param len 
                 * @return std::size_t 
                 */
                std::size_t Allocate(std::size_t len) {
                    if(!allocator_)// 检查是否已提供自定义的内存分配器,如果没有提供,则创建一个新的内存分配器
                        ownAllocator_ = allocator_ = RAPIDJSON_NEW(Allocator)();
                    size_t total = (3*len+7)*sizeof(Ch);// 计算为URI的各个部分分配的内存总量
                    // scheme_为分配内存的起始位置
                    scheme_ = static_cast<Ch*>(allocator_->Malloc(total));
                    *scheme_ = '\0';
                    // 使auth_指向scheme_后的下一个字符
                    auth_ = scheme_;
                    auth_++;
                    *auth_ = '\0';
                    // 使path_指向auth_后的下一个字符
                    path_ = auth_;
                    path_++;
                    *path_ = '\0';
                    // 使query_指向path_后的下一个字符
                    query_ = path_;
                    query_++;
                    *query_ = '\0';
                    // 使frag_指向query_后的下一个字符
                    frag_ = query_;
                    frag_++;
                    *frag_ = '\0';
                    // 使base_指向frag_后的下一个字符
                    base_ = frag_;
                    base_++;
                    *base_ = '\0';
                    // 使uri_指向base_后的下一个字符
                    uri_ = base_;
                    uri_++;
                    *uri_ = '\0';
                    return total;
                }
                void Free() {
                    if(scheme_) {
                        Allocator::Free(scheme_);
                        scheme_ = 0;
                    }
                }
                void Parse(const Ch* uri, std::size_t len) {
                    // start用于追踪URI字符串的起始位置
                    // pos1:标记scheme部分的结束位置
                    // pos2:标记authority部分的结束位置
                    std::size_t start = 0, pos1 = 0, pos2 = 0;
                    Allocate(len);// 分配内存来存储解析后的各个URI部分
                    // scheme部分是一个由字符、数字和一些符号组成的字符串,并且在URI中以':'结束
                    if(start < len) {
                        while(pos1<len) {
                            if(uri[pos1]==':')// 找到了scheme部分的结束位置
                                break;
                            pos1++;
                        }
                        // auth部分结束通常由'/'、'?'、'#'这些分隔符来标志
                        if(pos1!=len) {
                            while(pos2<len) {   
                                if(uri[pos2]=='/')
                                    break;
                                if(uri[pos2]=='?')
                                    break;
                                if(uri[pos2]=='#')
                                    break;
                                pos2++;
                            }
                            if(pos1<pos2) {// 意味着确实找到了一个有效的scheme部分
                                pos1++;// 希望scheme部分把':'也包含进去,所以+1
                                std::memcpy(scheme_, &uri[start], pos1*sizeof(Ch));// 拷贝scheme部分到当前URI对象的scheme_中
                                scheme_[pos1] = '\0';// 在scheme_末尾添加一个'\0',表示字符串结束
                                start = pos1;
                            }
                        }
                    }
                }

            }

            
    };
}

#endif