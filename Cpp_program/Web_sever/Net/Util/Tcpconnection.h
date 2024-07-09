#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "../../Base/NonCopyAble.h"
#include "../../Base/Timestamp.h"
#include "../../Base/Callback.h"
#include "../Http/HttpContent.h"
#include "Buffer.h"

/**
 * Acceptor(::socket   BindAddress) => TcpServer(Start()->Acceptor::Listen()) => Acceptor(handleRead()->::accept4()) => TcpServer::HandleNewConnection(有一个新用户连接,通过accept函数拿到connfd)
 * => TcpConnection设置回调 => 设置到Channel => Poller => Channel回调
 **/

namespace tiny_muduo{
    class EventLoop;
    class Socket;
    class Channel;
    class Address;
    class TcpConnection : public std::enable_shared_from_this<TcpConnection>, NonCopyAble{
        public:
            // 定义一个表示连接的不同状态的枚举类
            enum class ConnectionState{
                kConnecting, // 正在连接
                kConnected, // 已连接
                kDisconnecting, // 正在断开
                kDisconnected // 已断开
            };
            TcpConnection(EventLoop*, int, int, const Address&); // TcpConnection name = ip_port+connfd+tcpconnection_id
            ~TcpConnection();
            // 设置连接建立时的回调 可以使用右值引用或常引用
            void SetConnectionCallback(const ConnectionCallback& cb) {connection_callback_ = cb;}
            void SetConnectionCallback(ConnectionCallback&& cb) {connection_callback_ = cb;}
            // 设置消息到达时的回调 可以使用右值引用或常引用
            void SetMessageCallback(MessageCallback&& cb) {message_callback_ = cb;}
            void SetMessageCallback(const MessageCallback& cb) {message_callback_ = cb;}
            // 设置关闭时的回调 可以使用右值引用或常引用
            void SetCloseCallback(const CloseCallback& cb) {close_callback_ = cb;}
            void SetCloseCallback(CloseCallback&& cb) {close_callback_ = cb;}
            // 标记连接已建立(即设置连接已建立时对应的一些标志、状态)
            void ConnectionEstablished();
            // 关闭连接  半关闭 实际上是调用的Socket::ShutDownWrite()
            void Shutdown();
            // 将半关闭这个操作放在其所属的EventLoop的任务队列中去
            void ShutdownInLoop();
            // 检查连接是否处于关闭状态
            bool IsShutdown() const;
            // 获取当前连接的错误状态
            int GetError() const;
            // 连接析构函数,释放资源
            void ConnectionDestructor();
            // 处理连接关闭
            void HandleClose();
            // 处理连接错误
            void HandleError();
            // 处理收到的消息
            void HandleRead();// 内核缓冲区->用户缓冲区->输出消息
            // 处理可写事件
            void HandleWrite();// 输入消息->用户缓冲区->内核缓冲区
            void forceClose();// 服务器主动关闭连接
            void forceCloseInLoop();
            // 通过字符串发送数据
            void Send(const std::string&);
            // 将Send操作放入所属的EventLoop中
            void SendInLoop(const char*, int);
            // 指定更新时间戳 为了后续写入日志
            void UpdateTimestamp(Timestamp now){ timestamp_ = now;}
            // 获取时间戳
            Timestamp timestamp() const {return timestamp_;}
            // 获取当前TcpConnection的名称
            int name() const {ip_port_ + std::to_string(connfd_) + std::to_string(connection_id_);}
            // 获取当前TcpConnection的连接的ID
            int id() const {return connection_id_;}
            // 获取关联的事件循环loop
            EventLoop* loop() const { return loop_;}
            // 获取HTTP内容
            HttpContent* GetHttpContent() {return &content_;}
        private:
            EventLoop* loop_; // 当前连接所属的loop
            const std::string ip_port_;// 服务器的IP和端口
            int connfd_; // 当前连接的文件描述符
            int connection_id_; // 当前连接的ID
            ConnectionState state_; // 连接状态
            std::unique_ptr<Socket> TcpConnectionSocket_;
            std::unique_ptr<Channel> channel_;
            Buffer input_buffer_; // 接收缓冲区
            Buffer output_buffer_; // 发送缓冲区
            HttpContent content_; // 用于处理HTTP请求的内容
            bool shutdown_state_; // 表示连接是否已关闭
            Timestamp timestamp_; // 记录连接的时间戳
            ConnectionCallback connection_callback_;  // 上层回调 连接建立和连接销毁的回调函数
            MessageCallback message_callback_;        // 上层回调 消息到达的回调函数
            CloseCallback close_callback_;            // 连接关闭的回调函数
    };
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>; // 定义一个指向 TcpConnection 对象的共享指针类型   不能用unique_ptr
}
#endif