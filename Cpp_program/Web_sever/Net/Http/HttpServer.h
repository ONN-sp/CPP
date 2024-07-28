#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <functional>
#include <memory>
#include "../../Base/NonCopyAble.h"
#include "../Util/Tcpconnection.h"
#include "../Util/Tcpserver.h"
#include <string>

namespace tiny_muduo{
    static const double kIdleConnectionTimeOuts = 8.0;// 表示空闲连接的超时时间为8秒
    class HttpRequest;
    class HttpResponse;
    class Buffer;
    class EventLoop;
    class Address;
    class HttpServer : public NonCopyAble{
        public:
            using HttpResponseCallback = std::function<void(const HttpRequest&, HttpResponse&)>;
            HttpServer(EventLoop*, const Address&, bool auto_close_idleconnection = false, const std::string http_version = "HTTP/1.1");
            ~HttpServer();
            void Start() { server_->Start();}// 调用TcpServer::Start启动服务器
            // HTTP默认回调函数
            void HttpDefaultCallback(const HttpRequest&, HttpResponse&);
            // 处理空闲连接
            void HandleIdleConnection(std::weak_ptr<TcpConnection>&);
            // 连接回调函数
            void ConnectionCallback(const TcpConnectionPtr&);
            // 消息回调
            void MessageCallback(const TcpConnectionPtr&, Buffer*);
            // 设置HTTP响应回调
            void SetHttpResponseCallback(const HttpResponseCallback& response_callback) { response_callback_ = response_callback;}
            // 设置HTTP响应回调(移动语义方法)
            void SetHttpResponseCallback(HttpResponseCallback&& response_callback) { response_callback_ = response_callback;}
            // 设置服务器的线程数量
            void SetThreadNums(int thread_nums) { server_->SetThreadNums(thread_nums);}
            // 处理HTTP请求
            void DealWithRequest(const HttpRequest&, const TcpConnectionPtr&);
        private:
            EventLoop* loop_;
            std::unique_ptr<TcpServer> server_;
            bool auto_close_idleconnection_;// 是否自动关闭空闲连接的标志
            HttpResponseCallback response_callback_;
            std::string http_version_;// HTTP版本 字符串表示
    };
}

#endif