#include <string>
#include "../Util/Tcpserver.h"
#include "../Base/Logging/Logging.h"
#include "../Util/EventLoop.h"
#include "../Base/Address.h"

using namespace tiny_muduo;

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const Address &addr, const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.SetConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        
        server_.SetMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

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
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->Send(msg);
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main() {
    EventLoop loop;
    Address addr(9999);
    EchoServer server(&loop, addr, "EchoServer");
    server.Start();
    loop.loop();
    return 0;
}