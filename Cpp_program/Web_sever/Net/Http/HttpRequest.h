#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "../../Base/NonCopyAble.h"
#include "HttpRequestParseState.h"
#include <string>
#include <map>

namespace tiny_muduo{
    enum class Method{// HTTP请求方法
        kGet,
        kPost,
        kPut,
        kDelete,
        kTrace,
        kOptions,
        kConnect,
        kPatch
    };
    enum class Version{// HTTP版本
        kUnknown,
        kHttp10,
        kHttp11,
        kHttp20
    };
    class HttpRequest{
        public:
            HttpRequest();
            ~HttpRequest();
            // 解析请求方法
            bool ParseRequestMethod(const char*, const char*);
            // 解析请求行
            bool ParseRequestLine(const char*, const char*);
            // 解析请求头
            bool ParseHeaders(const char*, const char*, const char*);
            // 解析请求主体
            bool ParseBody(const char*, const char*);
            // 获取请求方法
            Method method() const { return method_;}
            // 获取请求路径
            const std::string& path() const { return path_;}
            // 获取查询字符串
            const std::string& query() const { return query_;}
            // 获取HTTP版本
            Version version() const { return version_;}
            // 获取请求头集合
            const std::map<std::string, std::string>& headers() const { return headers_;}
            // 交换HttpRequest对象
            void Swap(HttpRequest&);
            // 获取指定请求头的值
            std::string GetHeader(const std::string&) const;
        private:
            // 下面这几个成员变量就组成一个完整的Http请求报文(不考虑Body)
            Method method_;
            Version version_;
            std::string path_;
            std::string query_;
            std::map<std::string, std::string> headers_;
    };
}
#endif