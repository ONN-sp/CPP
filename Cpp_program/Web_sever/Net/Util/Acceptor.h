#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "../../Base/NonCopyAble.h"
#include <memory>
#include <functional>

namespace tiny_muduo{
    class EventLoop;
    class Address;
    class Channel;
    class Socket;
    // Acceptor类用于管理监听套接字的生命周期和新连接的接受
    class Acceptor : public NonCopyAble{
        public:
            using NewConnectionCallback = std::function<void(int, const Address&)>;// 参数为socket_fd   客户端地址
            Acceptor(EventLoop*, const Address&, bool);// 一个Acceptor loop->一个Acceptor thread
            ~Acceptor();
            // 处理新的连接 
            void handleRead();
            // 设置新连接的回调函数(左值引用版本)
            void SetNewConnectionCallback(const NewConnectionCallback&);
            // 设置新连接的回调函数(右值引用版本,用于移动语义)
            void SetNewConnectionCallback(NewConnectionCallback&&);
            // 使监听文件描述符进入监听状态
            void Listen();
        private:
            EventLoop* loop_; // 当前处理Acceptor的线程对应的EventLoop   // 其实传入的就是main()中的主loop
            std::unique_ptr<Socket> acceptSocket_;// 当前Acceptor对应的用于连接的套接字对象
            int id_lefd_; // 空闲文件描述符,用于处理文件描述符耗尽的情况
            std::unique_ptr<Channel> channel_; // 一个fd->一个Channel
            NewConnectionCallback new_connection_callback_;
    };
}
#endif