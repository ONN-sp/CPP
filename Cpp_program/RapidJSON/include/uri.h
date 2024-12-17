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

            
    };
}

#endif