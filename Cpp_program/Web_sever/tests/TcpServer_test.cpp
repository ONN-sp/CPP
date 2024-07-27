#include <string>
#include "../Net/Util/Tcpserver.h"
#include "../Base/Logging/Logging.h"
#include "../Net/Util/EventLoop.h"
#include "../Net/Util/Tcpconnection.h"
#include "../Base/Address.h"

using namespace tiny_muduo;

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const Address &addr, const std::string &address)
        : server_(loop, addr, address)
        , loop_(loop)
    {
        // 注册回调函数
        server_.SetConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        
        server_.SetMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));

        // 设置合适的subloop线程数量
        server_.SetThreadNums(3);
    }
    void Start()
    {
        server_.Start();
    }

private:
    // 连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr &conn)   
    {
        if (conn->connected())
        {
            LOG_INFO << "Connection UP : " << conn->fd();
        }
        else
        {
            LOG_INFO << "Connection DOWN : " << conn->fd();
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf)
    {
        std::string msg = buf->RetrieveAllAsString();
        conn->Send(msg.c_str(), msg.size());
        LOG_INFO << "Server recv message = " << msg;
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main() {
    EventLoop loop;
    Address addr("127.0.0.1", "9999");
    EchoServer server(&loop, addr, "EchoServer");
    server.Start();
    loop.loop();
    return 0;
}