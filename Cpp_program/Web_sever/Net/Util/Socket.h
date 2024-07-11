// 封装一个Socket RAII对象,自动管理socket生命周期,包括创建、绑定、监听、连接和关闭(shutdown 非close)等操作
#ifndef SOCKET_H
#define SOCKET_H

#include "../../Base/NonCopyAble.h"

namespace tiny_muduo{
    class Address;
    class Socket : public NonCopyAble{
        public:
            explicit Socket(int);
            ~Socket();
            int fd() const;// 获取文件描述符
            // 绑定监听文件描述符到指定地址  bind函数
            void BindAddress(const Address&, bool LookBackOnly = false);// 默认使用任意地址
            // 使监听文件描述符进入监听状态  listen()函数
            void Listen();
            // 接受新的连接 
            int Accept(Address*);
            // 接受新的连接 接受flags ::accept4
            int Accept4(Address*);
            // C/S中一方主动关闭,不会直接关闭读写操作,而是只关闭写操作,不关闭读操作   这样是为了收发数据的完整性
            void ShutDownWrite();
            // 设置套接字选项SO_KEEPALIVE,启用TCP长期连接
            int SetSockoptKeepAlive(bool);
            // 设置套接字选项SO_REUSEPORT,允许地址复用
            int SetSockoptReuseAddr(bool);
            // 设置套接字选项SO_REUSEADDR,允许地址复用
            int SetSockoptReusePort(bool);
            // 设置套接字选项TCP_NODELAY,禁用Nagle算法
            int SetSockoptTcpNoDelay(bool);
        private:
            int sockfd_;
    };
}
#endif