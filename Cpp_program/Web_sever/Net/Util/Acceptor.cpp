#include "../../Base/Address.h"
#include "Acceptor.h"
#include "../../Base/Logging/Logging.h"
#include "Socket.h"
#include "Channel.h"
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace tiny_muduo;

Acceptor::Acceptor(EventLoop* loop, const Address& address, bool reuseport)
    : loop_(loop),
      acceptSocket_(std::make_unique<Socket>(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP))),
      id_lefd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),// 打开 /dev/null 文件,并将返回的文件描述符赋给id_lefd_
      channel_(std::make_unique<Channel>(loop_, acceptSocket_->fd())){
        acceptSocket_->SetSockoptReuseAddr(true);// 设置地址复用选项
        acceptSocket_->SetSockoptReusePort(reuseport);// 设置地址复用选项
        acceptSocket_->SetSockoptKeepAlive(true);// 设置保持连接选项
        acceptSocket_->BindAddress(address);// 绑定Acceptor的监听文件描述符到指定地址
        channel_->SetReadCallback(std::bind(&Acceptor::handleRead, this));// 设置读回调函数为handleRead方法  即listen_fd_对应的channel的读事件回调为handleRead
      }

Acceptor::~Acceptor(){
    channel_->DisableAll();// 关闭Channel上的所有事件
    loop_->Remove(channel_.get());// 从EventLoop中移除Channel
    ::close(id_lefd_);// 关闭监听套接字
}

// 使监听文件描述符进入监听状态
void Acceptor::Listen(){
    acceptSocket_->Listen();
    channel_->EnableReading(); // 启用Acceptor对应的Channel的读事件监听
}
// 处理新的连接
void Acceptor::handleRead(){
    Address peer_addr;
    int connfd = acceptSocket_->Accept4(&peer_addr);
    if(connfd < 0){
        if(errno == EMFILE){// 此时是因为文件描述符耗尽才建立连接失败
            ::close(id_lefd_);// 先关闭之前创建的空闲文件描述符 此时就有一个新的文件描述符可以用了(因此一定可以再次成功调用accept,只有成功调用后(成功调用后,文件描述符又耗尽了),此时操作系统就会自动会在内部关闭一些不再需要的文件描述符)
            id_lefd_ = ::accept(acceptSocket_->fd(), NULL, NULL);// 重新尝试通过::accept()接受一个连接,以让操作系统释放一些不再使用的文件描述符(当文件描述符耗尽时,再次调用会让它自动释放不用的文件描述符)
            ::close(id_lefd_);// 此时id_lefd_是重新建立连接的通信文件描述符,此时需要立即关闭重新接受的文件描述符.这样做的目的是确保不会因为文件描述符泄露(没close就会泄露)而导致资源浪费或系统限制
            id_lefd_ = ::open(::open("/dev/null", O_RDONLY | O_CLOEXEC));
        }
        return;
    }
    else{
        if(new_connection_callback_)
            // 调用新连接的回调函数(连接完成后的后续操作(写在一个回调函数中了))
            new_connection_callback_(connfd, peer_addr);// SetSockAddrInet4函数已经把对端的ip port都设置在peer_addr.addr_中了
        else
          ::close(connfd);// 成功建立了连接,但是新建的连接后续没有任何操作了(不读不写了),即这个连接结束了,此时就可以关闭这个通信文件描述符了
    }
    bool OnOrOff = connfd > 0;// 确保接收的文件描述符有效
    if(acceptSocket_->SetSockoptKeepAlive(OnOrOff)==-1){
        LOG_ERROR << "Acceptor::NewConnection SetSockoptKeepAlive failed"; // 设置保持连接失败
        ::close(connfd); // 关闭连接
        return; 
    }
    if (acceptSocket_->SetSockoptTcpNoDelay(OnOrOff) == -1) {
        LOG_ERROR << "Acceptor::NewConnection SetSockoptTcpNoDelay failed"; // 设置禁用Nagle算法失败
        ::close(connfd); // 关闭连接
        return;
    }
}
// 设置新连接的回调函数(左值引用版本)
void Acceptor::SetNewConnectionCallback(const NewConnectionCallback& callback) {
    new_connection_callback_ = callback;
}
// 设置新连接的回调函数(右值引用版本,用于移动语义)
void Acceptor::SetNewConnectionCallback(NewConnectionCallback&& callback) {
    new_connection_callback_ = std::move(callback);
}

