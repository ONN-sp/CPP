#include "HttpResponse.h"

using namespace tiny_muduo;

HttpResponse::HttpResponse(bool close_connection, const std::string& server_name, const std::string& http_version)// 根据指定参数设置是否关闭连接
    : type_("text/plain"),
      http_version_(http_version),
      server_name_(server_name),
      close_connection_(close_connection)
      {}

HttpResponse::~HttpResponse(){}

// 将响应内容附加到缓冲区
void HttpResponse::AppendToBuffer(Buffer* buffer){
    char buf[32] = {0};
    // 格式化状态行并附加到缓冲区
    snprintf(buf, sizeof(buf), "%s %d ", http_version_.c_str(), status_code_);
    buffer->Append(buf);
    buffer->Append(status_message_);
    buffer->Append(kCRLF);
    // 根据是否关闭连接来确定添加相应的连接字段
    if(close_connection_)
        buffer->Append("Connection: close\r\n");
    else{
        snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
        buffer->Append(buf);
        buffer->Append("Connection: Keep-Alive\r\n");
    }
    // 添加内容类型字段
    buffer->Append("Content-Type: ");
    buffer->Append(type_);
    buffer->Append(kCRLF);
    // 添加服务器名称字段
    buffer->Append("Server: ");
    buffer->Append(server_name_);
    buffer->Append(kCRLF);
    buffer->Append(kCRLF);// Body内容之前有一个空行
    // 添加响应体内容
    buffer->Append(body_);
}