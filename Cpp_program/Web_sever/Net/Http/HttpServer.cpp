#include "HttpServer.h"
#include "../../Base/Logging/Logging.h"

using namespace tiny_muduo;

HttpServer::HttpServer(EventLoop* loop, const Address& address, bool auto_close_idleconnection)
    : loop_(loop),
      server_(std::make_unique<TcpServer>(loop, address, "HttpServer")),
      auto_close_idleconnection_(auto_close_idleconnection){
        // 把HttpServer的ConnectionCallback  MessageCallback传给对应的TcpServer(相对于在TcpServer上再封装一层)
        server_->SetConnectionCallback(std::bind(&HttpServer::ConnectionCallback, this, std::placeholders::_1));
        server_->SetMessageCallback(std::bind(&HttpServer::MessageCallback, this, std::placeholders::_1, std::placeholders::_2));
        SetHttpResponseCallback(std::bind(&HttpServer::HttpDefaultCallback, this, std::placeholders::_1, std::placeholders::_2));
        LOG_INFO << "HttpServer listening on " << address.IpPortToString();
      }

HttpServer::~HttpServer(){}

void HttpServer::HttpDefaultCallback(const HttpRequest& request, HttpResponse& response){
    response.SetStatusCode(HttpStatusCode::k404NotFound);
    response.SetStatusMessage("Not Found");
    response.SetCloseConnection(true);// 设置关闭连接
}

void HttpServer::HandleIdleConnection(std::weak_ptr<TcpConnection>& connection){
    TcpConnectionPtr conn(connection.lock());// 从weak_ptr创建一个shared_ptr获取TCP连接的共享指针
    if(conn){
        if(Timestamp::AddTime(conn->timestamp(), kIdleConnectionTimeOuts) < Timestamp::Now())
            conn->Shutdown();// 超过空闲超时时间,就关闭连接
        else// 重新安排一个超时检查   即HandleIdleConnection()在8秒后被再次执行 
            loop_->RunAfter(kIdleConnectionTimeOuts, std::move(std::bind(&HttpServer::HandleIdleConnection, this, connection)));
    }
}

// 连接回调函数,如果启用了自动关闭空闲连接,则安排空闲超时检查
void HttpServer::ConnectionCallback(const TcpConnectionPtr& connection){
    if(auto_close_idleconnection_)
        loop_->RunAfter(kIdleConnectionTimeOuts, std::bind(&HttpServer::HandleIdleConnection, this, std::weak_ptr<TcpConnection>(connection)));
}
// 有
void HttpServer::MessageCallback(const TcpConnectionPtr& connection, Buffer* buffer){
    HttpContent* content = connection->GetHttpContent();// 获取连接对应的HttpContent对象,用于解析HTTP内容
    if(auto_close_idleconnection_)
        connection->UpdateTimestamp(Timestamp::Now());
        if(connection->IsShutdown())
            return;
        if(!content->ParseContent(buffer)){// 解析失败
            connection->Send("400 Bad Request\r\n\r\n");// 两个换行符是表示只发送响应头,后面的头部信息和Body都没有
            connection->Shutdown();
        }
        if(content->GetCompleteRequest()){// 完整解析成功
            DealWithRequest(content->request(), connection);// 处理该连接的HTTP请求
            content->ResetContentState();// 重置内容状态
        }
}
 // 处理HTTP请求
void HttpServer::DealWithRequest(const HttpRequest& request, const TcpConnectionPtr& connection){
    std::string connection_state = std::move(request.GetHeader("Connection"));// 查找头部字段Connection对应的value,即连接状态
    bool close = (connection_state=="Close" || (request.version()==Version::kHttp10&&connection_state!="Keep-Alive"));// 确定是否需要关闭连接
    HttpResponse response(close);// close决定了这个给客户端的响应告诉对方是否关闭连接
    response_callback_(request, response);// 执行响应回调
    Buffer buffer;
    response.AppendToBuffer(&buffer);// 将响应添加到缓冲区  response->Buffer
    connection->Send(&buffer);// Buffer->TcpConnection(将数据发送)->fd对应的内核缓冲区->客户端
    if(response.CloseConnection())
        connection->Shutdown();
}