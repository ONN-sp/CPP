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
            /**
             * @brief 将输入的URI字符串拆解成不同的组成部分,如scheme、authority、path、query和fragment,并存储到相应的成员变量中
             * 
             * @param uri 
             * @param len 
             */
            void Parse(const Ch* uri, std::size_t len) {
                std::size_t start = 0, pos1 = 0, pos2 = 0;
                Allocate(len);// 分配内存来存储解析后的各个URI部分
                // scheme部分是一个由字符、数字和一些符号组成的字符串,并且在URI中以':'结束
                // pos1:标记scheme部分的结束位置
                // scheme部分结束通常由':'结束符来标志
                if(start < len) {
                    while(pos1<len) {
                        if(uri[pos1]==':')// 找到了scheme部分的结束位置
                            break;
                        pos1++;
                    }
                    // auth部分结束通常由'/'、'?'、'#'这些结束符来标志
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
                    // 解析auth部分
                    auth_ = scheme_+GetSchemeStringLength();
                    auth_++;
                    *auth_ = '\0';
                    // 通过检查URI中是否由'//'来确定auth的开始位置
                    if(start<len-1 && uri[start]=='/' && uri[start+1]=='/') {
                        pos2 = start+2;
                        while(pos2<len) {
                            if(uri[pos2]=='/')
                                break;
                            if(uri[pos2]=='?')
                                break;
                            if(uri[pos]=='#')
                                break;
                            pos2++;
                        }
                        // pos2为authority部分的结束位置,通过检查结束符'/'、'?'、'#'实现的
                        std::memcpy(auth_, &uri[start], (pos2-start)*sizeof(Ch));
                        auth_[pos2-start] = '\0';
                        start = pos2;
                    }
                    // 解析path部分
                    path_ = auth_+GetAuthStringLength();// 计算当前解析得到的URI对象的uri_中的path_部分的起始地址
                    path_++;
                    *path_ = '\0';
                    if(start < len) {
                        pos2 = start;
                        while(pos2<len) {
                            if(uri[pos2]=='?')
                                break;
                            if(uri[pos2]=='#')
                                break;
                            pos2++;
                        }
                        // pos2为path部分的结束位置,通过检查结束符'?'、'#'实现的
                        if(start!=pos2) {
                            std::memcpy(path_, &uri[start], (pos2-start)*sizeof(Ch));
                            path_[pos2-start] = '\0';
                            if(path_[0]=='/')// 如果是绝对路径就调用RemoveDotSegments()来规范化路径
                                RemoveDotSegments();// 规范化路径
                            start = pos2;
                        }
                    }
                    // 解析query部分
                    query_ = path_+GetPathStringLength();
                    query_++;
                    *query_ = '\0';
                    // query是以'?'开始的
                    if(start<len&&uri[start]=='?') {
                        pos2 = start+1;
                        while(pos2<len) {
                            if(uri[pos2]=='#')
                                break;
                            pos2++;
                        }
                        // pos2为query部分的结束位置,通过检查结束符'#'实现的
                        if(start!=pos2) {
                            std::memcpy(query_, &uri[start], (pos2-start)*sizeof(Ch));
                            query_[pos2-start] = '\0';
                            start = pos2;
                        }
                    }
                    // 解析frag部分
                    frag_ = query_ + GetQueryStringLength();
                    frag_++;
                    *frag_ = '\0';
                    // fragment是'#'开始的
                    if(start<len&&uri[start]=='#') {
                        std::memcpy(frag_, &uri[start], (len-start)*sizeof(Ch));
                        frag_[len-start] = '\0';
                    }
                    base_ = frag_ + GetFragStringLength()+1;// 当前URI对象的base_地址位于frag_部分后
                    SetBase();// 设置当前URI对象的base_的内容
                    uri_ = base_ + GetBaseStringLength()+1;// 当前URI对象的uri_地址位于base_部分后
                    SetUri();// 设置当前URI对象的uri_的内容
                }
            }
            /**
             * @brief 负责构建当前URI对象的base=scheme+auth+path+query
             * 
             */
            void SetBase() {
                Ch* next = base_;
                std::memcpy(next, scheme_, GetBaseStringLength()*sizeof(Ch));
                next += GetSchemeStringLength();
                std::memcpy(next, auth_, GetAuthStringLength()*sizeof(Ch));
                next += GetAuthStringLength();
                std::memcpy(next, path_, GetPathStringLength()*sizeof(Ch));
                next += GetPathStringLength();
                std::memcpy(next, query_, GetQueryStringLength()*sizeof(Ch));
                next += GetQueryStringLength();
                *next = '\0';
            }
            /**
             * @brief 负责构建当前URI对象的uri=scheme+auth+path+query+frag
             * 
             */
            void SetUri() {
                Ch* next = uri_;
                std::memcpy(next, base_, GetBaseStringLength()*sizeof(Ch));
                next += GetBaseStringLength();
                std::memcpy(next, frag_, GetFragStringLength()*sizeof(Ch));
                next += GetFragStringLength();
                *next = '\0';
            }
            /**
             * @brief 将一个部分复制from到另一个位置to,并返回下一个可用位置
             * 
             * @param to 
             * @param from 
             * @param len 
             * @return Ch* 
             */
            Ch* CopyPart(Ch* to, Ch* from, std::size_t len) {
                    RAPIDJSON_ASSERT(to!=0);
                    RAPIDJSON_ASSERT(from!=0);
                    std::memcpy(to, from, len*sizeof(Ch));
                    to[len] = '\0';
                    Ch* next = to+len+1;
                    return next;
            }
            /**
             * @brief 用于处理和规范化URI对象的路径部分中的'.'(当前目录)和'..'(上一目录)
             * 如果路径段是'..',则表示上一目录,需要回退到上一个目录
             * 如果路径段是'.',表示当前目录,不需要做任何处理,直接跳过该段
             */
            void RemoveDotSegments() {
                std::size_t pathlen = GetPathStringLength();// 获取uri中路径部分的长度
                std::size_t pathpos = 0;// 用来跟踪当前正在处理的路径段位置
                std::size_t newpos = 0;// 用来表示规范化后字符串已经填充的位置,即下一个待填充的位置
                while(pathpos < pathlen) {
                    size_t slashpos = 0;// slashpos用来计算当前路径段的长度
                    while((pathpos+slashpos)<pathlen) {
                        if(path_[pathpos+slashpos]=='/')// 每次遇到'/',表示路径的下一部分的开始,因此可用计算一个路径段的长度
                            break;
                        slashpos++;
                    }
                    // 检查当前路径段是否为'..',若是就回退到上一个路径段
                    if(slashpos==2&&path_[pathpos]=='.'&&path[pathpos+1]=='.') {
                        RAPIDJSON_ASSERT(newpos==0||path_[newpos-1]=='/');
                        size_t lastslashpos = newpos;// 用lastslashpos来返回上一个有效路径段的结束位置
                        if(lastslashpos > 1) {
                            lastslashpos--;
                            while(lastslashpos>0) {
                                if(path_[lastslashpos-1]=='/')
                                    break;
                                lastslashpos--;
                            }
                            newpos = lastslashpos;// 回退到上一个有效路径段结束位置
                        }
                    }
                    else if(slashpos==1&&path_[pathpos]=='.'){// 检查当前路径段是否为'.',若是就什么都不做,直接丢弃

                    }
                    else {// 其余情况则直接将其移动到新的路径中(newpos)
                        RAPIDJSON_ASSERT(newpos<=pathpos);
                        std::memmove(&path_[newpos], &path_[pathpos], slashpos*sizeof(Ch));// 复制每一个规范化后的路径段到此时的newpos位置处
                        newpos += slashpos;
                        if((pathpos+slashpos) < pathlen) {
                            path_[newpos] = '/';
                            newpos++;
                        }
                    }
                    pathpos += slashpos + 1;// 跳到下一个待处理的路径段
                }
                path_[newpos] = '\0';// 新的路径段末尾添加'\0'
            }
            Ch* uri_;// scheme_+auth_+path_+query_+frag_
            Ch* base_;// scheme_+auth_+path_+query_
            Ch* scheme_;// uri字符串的scheme部分
            Ch* auth_;// uri字符串的authority部分
            Ch* path_;// uri字符串的path部分
            Ch* query_;// uri字符串的query部分
            Ch* frag_;// uri字符串的fragment部分
            Allocator* allocator_;
            Allocator* ownAllocator_;            
    };
    typedef GenericUri<Value> Uri;
}
#endif