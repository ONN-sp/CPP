#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "../../Base/NonCopyAble.h"
#include "../../Base/Callback.h"
#include <unordered_map>
#include <functional>
#include <string>
#include <utility>

namespace tiny_muduo{
    class Address; 
    class EventLoop;
    class EventLoopThreadPool;
    class Acceptor;
    class TcpServer : public NonCopyAble{
        public:
            using ThreadInitCallback = std::function<void(EventLoop*)>;
            // Option枚举类，用于指定端口复用选项  用于传入Acceptor
            enum class Option
            {
                kNoReusePort,
                kReusePort,
            };
             
            TcpServer(EventLoop*, const Address&, const std::string&, Option option = Option::kNoReusePort);
            ~TcpServer();
            void Start();// 启动服务器：开始监听和处理新连接
             // 设置连接回调函数，可以使用右值引用或常引用   ConnectionCallback是用户自定义传入的
            void SetConnectionCallback(ConnectionCallback&& cb) {connection_callback_ = cb;}
            void SetConnectionCallback(const ConnectionCallback& cb) {connection_callback_ = cb;}
            // 设置消息回调函数，可以使用右值引用或常引用  MessageCallback是用户自定义传入的
            void SetMessageCallback(MessageCallback&& cb) {message_callback_ = cb;}
            void SetMessageCallback(const MessageCallback& cb) {message_callback_ = cb;}
            void SetThreadNums(int);// // 设置线程数量  其实就是EventLoopThreadPool中SetThreadNums的上层封装
            void SetThreadInitCallback(const ThreadInitCallback& cb) {threadInit_callback_ = cb;}// 设置线程回调函数
            inline void HandleClose(const TcpConnectionPtr&);// 处理连接销毁
            inline void HandleCloseInLoop(const TcpConnectionPtr&);// 将连接销毁操作放入其所属的EventLoop中
            inline void HandleNewConnection(int, const Address&);// 处理新连接
        private:
            using ConnectionMap = std::unordered_map<int, TcpConnectionPtr>;// 存储活跃连接的映射,键为TcpConnectionPtr指针对应的TcpConnection对象的名称,值为TcpConnection智能指针
            EventLoop* loop_;// 其实传入的就是main()中的主loop
            int next_conn_id;// 下一个连接的ID:用于生成新的连接ID(成功连接一个就+1)  一个TcpConnection对应一个ID
            std::unique_ptr<Acceptor> acceptor_;// 不用shared_ptr,是为了确保只有Tcpserver控制Acceptor的生命周期(确保Acceptor只会在Tcpserver的析构函数中被释放),即只有他独占,没有其它地方共享,所以设计为unique_ptr
            std::shared_ptr<EventLoopThreadPool> threads_;// 管理EventLoopThreadPool的共享所有权,可能在多个地方使用
            const std::string ip_port_;// 服务器的IP和端口
            const std::string name_;
            ConnectionCallback connection_callback_;
            MessageCallback message_callback_;
            ThreadInitCallback threadInit_callback_;
            ConnectionMap connections_;
    };
}

#endif