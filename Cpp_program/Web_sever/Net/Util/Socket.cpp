#include "Socket.h"
#include <sys/socket.h>
#include "../../Base/Logging/Logging.h"
#include "../../Base/Address.h"
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h> // sockaddr_in
#include <cassert>
#include <unistd.h>
#include <arpa/inet.h>

using namespace tiny_muduo;

Socket::Socket(int sockfd)
    : sockfd_(sockfd)
    {}

Socket::~Socket(){
    if(::close(sockfd_) < 0)// close()失败返回-1  成功返回0
        LOG_ERROR << "Close failed";
}
// 绑定监听文件描述符到指定地址   LookBackOnly参数表示是否绑定为回环地址
void Socket::BindAddress(const Address& addr, bool LookBackOnly){// 因为Address没像muduo的InetAddress那样,把sockaddr_in的初始化放在InetAddress的构造函数中,因此这里要手动初始化
    struct sockaddr_in address;
    ::memset(&address, 0, sizeof(address)); //<=>bzero((char*)&address, sizeof(address)); 清空地址结构体
    address.sin_family = AF_INET; // 设置地址族为IPv4
    address.sin_addr.s_addr = LookBackOnly ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY); // 绑定到任意地址或回环地址
    address.sin_port = htons(static_cast<uint16_t>(addr.Port())); // 设置端口号
    int ret = ::bind(sockfd_, (struct sockaddr*)(&address), sizeof(address)); // 绑定套接字到地址
    assert(ret != -1); // 检查绑定是否成功
    (void)ret; // 防止编译器警告未使用的变量
}
// 使监听文件描述符进入监听状态
void Socket::Listen(){
    int ret = ::listen(sockfd_, SOMAXCONN); // 开始监听
    assert(ret != -1);
    (void)ret;// 防止编译器警告变量未使用
}
// 接受连接
int Socket::Accept(Address* peeraddr){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int connfd = ::accept(sockfd_, (struct sockaddr*)&addr, &addr_len);
    if(connfd >= 0)
        peeraddr->SetSockAddrInet4(addr);
    return connfd;
}
// 接受连接
int Socket::Accept4(Address* peeraddr){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int connfd = ::accept4(sockfd_, (struct sockaddr*)&addr, &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);// 默认是阻塞的,这里设置非阻塞+关闭执行选项
    if(connfd >= 0)
        peeraddr->SetSockAddrInet4(addr);
    return connfd;
}
// 关闭可写操作(半关闭)  注意不是直接close,直接close会把可读可写操作都关了
void Socket::ShutDownWrite(){
    if(::shutdown(sockfd_, SHUT_WR) < 0)
        LOG_ERROR << "ShutdownWrite failed";
}
// 设置套接字选项SO_REUSEPORT,允许端口复用
int Socket::SetSockoptReusePort(bool on) {
    int option_val = on ? 1 : 0;
    return setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                      &option_val, static_cast<socklen_t>(sizeof(option_val)));
}
// 设置套接字选项O_REUSEADDR,允许地址复用
int Socket::SetSockoptReuseAddr(bool on) {
    int option_val = on ? 1 : 0;
    return setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                      &option_val, static_cast<socklen_t>(sizeof(option_val)));
}
// 设置套接字选项SO_KEEPALIVE,启用TCP长期连接
int Socket::SetSockoptKeepAlive(bool on) {
    int option_val = on ? 1 : 0;
    return setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                      &option_val, static_cast<socklen_t>(sizeof(option_val)));
}
// 设置套接字选项TCP_NODELAY,禁用Nagle算法
int Socket::SetSockoptTcpNoDelay(bool on) {
    int option_val = on ? 1 : 0;
    return setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                      &option_val, static_cast<socklen_t>(sizeof(option_val)));
}
// 返回当前套接字的文件描述符
int Socket::fd() const {
    return sockfd_;
}