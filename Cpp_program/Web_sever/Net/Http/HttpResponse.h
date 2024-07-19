#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "HttpRequest.h"
#include "../Util/Buffer.h"
#include "../../Base/NonCopyAble.h"

namespace tiny_muduo{
    enum class HttpStatusCode{// HTTP请求后回复的状态码
        k100Continue = 100,
        k200OK = 200,
        k400BadRequest = 400,
        k403Forbidden = 403,
        k404NotFound = 404,
        k500InternalServerErrno = 500
    };
    class HttpResponse : public NonCopyAble{
        public:
            HttpResponse(bool, const std::string& server_name = "Drew Jun", const std::string& http_version = "HTTP/1.1");
            ~HttpResponse();
            // 设置HTTP状态码
            void SetStatusCode(HttpStatusCode status_code) { status_code_ = status_code;}
            // 设置状态信息
            void SetStatusMessage(const std::string& status_message) { status_message_ = status_message;}
            // 设置是否关闭连接
            void SetCloseConnection(bool close_connection) { close_connection_ = close_connection;}
            // 设置内容类型
            void SetBodyType(const std::string& type) { type_ = type;}
            // 设置响应体内容
            void SetBody(const std::string& body) { body_ = body;}
            // 将响应内容附加到缓冲区
            void AppendToBuffer(Buffer*);
            // 返回是否关闭连接的标志
            bool CloseConnection() { return close_connection_;}
        private:
            std::string server_name_;// 服务器名称
            HttpStatusCode status_code_;// HTTP状态码
            std::string status_message_;// 状态消息
            std::string body_;// 响应体内容
            std::string type_;// 响应体内容类型
            bool close_connection_;// 是否关闭连接的标志
            std::string http_version_;// HTTP版本
    };
}
#endif