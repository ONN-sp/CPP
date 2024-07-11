#include "Tcpconnection.h"
#include "Channel.h"
#include "Socket.h"
#include "EventLoop.h"
#include "../../Base/Logging/Logging.h"
#include <sys/socket.h>
#include "../../Base/Address.h"
#include <cerrno>
#include <unistd.h>

using namespace tiny_muduo;

TcpConnection::TcpConnection(EventLoop* loop, int connfd, int id, const Address& address)
    : loop_(loop),
      connfd_(connfd),
      connection_id_(id),
      state_(ConnectionState::kConnecting),
      ip_port_(address.IpPortToString()),// 为了构造当前TcpConnection的名称
      TcpConnectionSocket_(std::make_unique<Socket>(connfd)),// connfd是一个已连接的文件描述符
      channel_(std::make_unique<Channel>(loop, connfd)),// TcpConnection是在Tcpserver::HandleNewConnection中新建的,此时的connfd_是accept(已连接)之后的文件描述符
      highWaterMark_(64 * 1024 * 1024){
        // 下面几行就注册了Channel::HandleEventWithGuard中的四种回调函数
        channel_->SetReadCallback(std::bind(&TcpConnection::HandleRead, this));
        channel_->SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));
        channel_->SetErrorCallback(std::bind(&TcpConnection::HandleError, this));
        channel_->SetCloseCallback(std::bind(&TcpConnection::HandleClose, this)); // 这个channel_的关闭回调和服务端被动关闭调用HandleClose是两码事
        //  Tcp的KeepAlive用在这是最合理的,因为这里才是一个新的Tcp连接的起点  Acceptor中的bind listen是使用TCP协议,而不是一个真正的TCP连接的起点
        TcpConnectionSocket_->SetSockoptKeepAlive(true); 
    }

TcpConnection::~TcpConnection(){
    ::close(connfd_);
}

void TcpConnection::ConnectionEstablished(){
    state_ = ConnectionState::kConnected;
    channel_->Tie(shared_from_this());// 确保Tcpconnection的生命期比它里面定义的channel_的生命期长
    channel_->EnableReading();// 向epoller注册channel的可读事件   
    connection_callback_(shared_from_this());// 执行连接关闭回调函数
}

void TcpConnection::Shutdown(){
    if(state_ == ConnectionState::kConnected){// 确保关闭前处于已连接
        state_ = ConnectionState::kDisconnected;
        loop_->RunOneFunc(std::bind(&TcpConnection::ShutdownInLoop, this)); // 为什么要放入到所属的Loop中执行?  如果 TcpConnection::shutdown 函数直接在当前调用线程中执行关闭操作，而这个线程可能不是 TcpConnection 所属的 EventLoop 线程，就会导致线程安全问题
    }
}

void TcpConnection::ShutdownInLoop(){
    if(!channel_->IsWriting())
        TcpConnectionSocket_->ShutDownWrite();
}

bool TcpConnection::IsShutdown() const {
    return state_ == ConnectionState::kDisconnecting;
}
// 返回当前套接字的错误状态  套接字出错才会调用当前函数
int TcpConnection::GetError() const {
    // 定义一个整数变量,用于存储套接字选项的返回值
    int opt;
    socklen_t opt_len = static_cast<socklen_t>(sizeof(opt));
    // 调用getsockopt,获取套接字的选项值SO_ERROR
    // 调用失败返回值<0,然后返回当前线程的错误码errno   errno用于存储最近一次系统调用失败的错误码
    if(::getsockopt(connfd_, SOL_SOCKET, SO_ERROR, &opt, &opt_len) < 0)
        return errno;
    else    
        return opt;// 套接字的错误状态
}
// 错误回调函数
void TcpConnection::HandleError() {
  LOG_ERROR << "TcpConnection::HandleError" << " : " << ErrorToString(GetError());
}
// 连接析构函数,释放资源
void TcpConnection::ConnectionDestructor(){
    if(state_ == ConnectionState::kDisconnecting || state_ == ConnectionState::kConnected){
        state_ = ConnectionState::kDisconnected;
        channel_->DisableAll();
        connection_callback_(shared_from_this());
    }
    loop_->RemoveChannel(channel_.get());
}
// 处理连接关闭
void TcpConnection::HandleClose(){
    state_ = ConnectionState::kDisconnected;
    channel_->DisableAll();
    TcpConnectionPtr guard(shared_from_this());
    connection_callback_(guard);// 执行连接关闭回调函数
    close_callback_(guard);// 执行关闭连接的回调 执行的是TcpServer::HandleClose回调方法 (close_callback_在TcpServer::HandleNewConnection中被设置)
}

// 当前服务器主动关闭连接(在实际应用中不常用,因为一般都是用的被动关闭)
void TcpConnection::forceClose()
{
  if (state_ == ConnectionState::kConnected || state_ == ConnectionState::kDisconnecting)
  {
    state_ = ConnectionState::kDisconnecting;
    loop_->QueueOneFunc(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

// 当前服务器主动关闭连接(在实际应用中不常用,因为一般都是用的被动关闭)
void TcpConnection::forceCloseInLoop()
{
  if (state_ == ConnectionState::kConnected || state_ == ConnectionState::kDisconnecting)
    HandleClose();
}

// 处理收到的消息
void TcpConnection::HandleRead(){
    int read_size = input_buffer_.ReadFd(connfd_);// 内核缓冲区读到用户缓冲区input_buffer_
    if(read_size > 0)//   // 已建立连接的用户有可读事件发生了 调用用户自定义的回调操作onMessage    shared_from_this就是获取了TcpConnection的智能指针
        message_callback_(shared_from_this(), &input_buffer_);
    else if(read_size == 0)// 客户端断开  这种是被动关闭,即对方关闭  服务端通过读取数据为0来判断对方关闭了
        HandleClose();
    else{
        HandleError();
        LOG_ERROR << "TcpConnection::HandleMessage read failed";
    }
}
// 处理可写事件
void TcpConnection::HandleWrite(){
    if(channel_->IsWriting()){
        int len = output_buffer_.readablebytes();// 用户缓冲区的可读缓冲区的大小
        int remain = len;
        int send_size = static_cast<int>(::write(connfd_, output_buffer_.Peek(), remain));// 没向内核缓冲区写完send_size=已写入的数值
        if(send_size < 0){// 说明写失败
            if(errno != EWOULDBLOCK)// connfd_在Aceeptor中被设置成的非阻塞,所以可能由于当前操作无法立即完成而被阻塞导致send_size<0,此时其实不是真正的错误,所以写日志的时候给区分开了
                LOG_ERROR << "TcpConnection:HandleWirte write failed";
            return;
        }
        remain -= send_size;// 内核缓冲区写满后,还剩下的没写进去的数据
        output_buffer_.Retrieve(send_size);// 因为已经写了send_size(即读出了send_size的数据),所以output_buffer_的readIndex要往前移动(即从output_buffer_中提取send_size长的空间)
        if(output_buffer_.readablebytes() == 0){// output_buffer_内容写完了  需要注意:如果没写完,即产生可写事件,还会继续回调HandleWrite
            channel_->DisableWriting();// 不关闭就会busy loop
            if(state_ == ConnectionState::kDisconnecting)
                Shutdown();
        }
    }
}

void TcpConnection::Send(const std::string& message){
    if(state_ == ConnectionState::kConnected){
        if(loop_->IsInThreadLoop())// 判断判断当前EventLoop对象是否在自己所属的线程里  如果是,则说明这是一个单reactor(loop(thread))的情况 用户调用conn->send时 loop_即为当前线程
            SendInLoop(message.c_str(), message.size());
        else
            loop_->RunOneFunc(std::bind(&TcpConnection::SendInLoop, this, message.c_str(), message.size()));
    }
}

void TcpConnection::Send(const char* message, int len){
    if(state_ == ConnectionState::kConnected){
        if(loop_->IsInThreadLoop())// 判断判断当前EventLoop对象是否在自己所属的线程里  如果是,则说明这是一个单reactor(loop(thread))的情况 用户调用conn->send时 loop_即为当前线程          
            SendInLoop(message, len);
        else
            loop_->RunOneFunc(std::bind(&TcpConnection::SendInLoop, this, message, len));
    }
}

void TcpConnection::Send(Buffer* buffer){
    if(state_ == ConnectionState::kConnected){
        if(loop_->IsInThreadLoop())// 判断判断当前EventLoop对象是否在自己所属的线程里  如果是,则说明这是一个单reactor(loop(thread))的情况 用户调用conn->send时 loop_即为当前线程
            SendInLoop(buffer->Peek(), buffer->readablebytes());
        else
            loop_->RunOneFunc(std::bind(&TcpConnection::SendInLoop, this, buffer->Peek(), buffer->readablebytes()));
    }
}

// 直接将给定的非Buffer中的数据message发送给内核缓冲区
void TcpConnection::SendInLoop(const char* message, int len){
    int remain = len;
    int send_size = 0;
    if (!channel_->IsWriting() && output_buffer_.readablebytes() == 0) {// 表示channel_第一次开始写数据或缓冲区没有待发数据
        send_size = static_cast<int>(::write(connfd_, message, len));
        if (send_size >= 0) {
            remain -= send_size;// message没写完的字节数  此时内核缓冲区满了
            if(remain == 0 && writeCompleteCallback_)// 写完了  调用写完成回调(也叫低水位回调)    
                loop_->QueueOneFunc(std::bind(writeCompleteCallback_, shared_from_this()));
        }
        else {
            if (errno != EWOULDBLOCK)
                LOG_ERROR << "TcpConnection::Send write failed";
            return;
        }
        /**
         * 下面程序处理的是当前这一次write并没有把数据全部发送出去,而剩余的数据需要保存到缓冲区当中
         * 然后给channel注册EPOLLOUT事件，Poller发现tcp的发送缓冲区有空间后会通知
         * 相应的sock->channel，调用channel对应注册的writeCallback_回调方法，
         * channel的writeCallback_实际上就是TcpConnection设置的handleWrite回调，
         * 把发送缓冲区outputBuffer_的内容全部发送完成
         **/
        if (remain > 0) {// message没写完的部分
            int oldLen = output_buffer_.readablebytes();// 目前发送缓冲区剩余的待发送的数据的长度
            if(oldLen + remain >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)// oldLen < highWaterMark_保证了只在上升沿触发一次
                loop_->QueueOneFunc(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen+remain));
            output_buffer_.Append(message + send_size, remain);// 将message没写完的部分直接添加到output_buffer_中(前面已写的部分没用output_buffer_)    不能在此处直接发送,因为之前output_buffer_可能有剩的数据,如果此处直接发送就会乱序
            if (!channel_->IsWriting()) 
                channel_->EnableWriting(); // 一定要注册写事件,因为此时还没写完,所以还要继续写(继续通过触发可写事件回调TcpConnection::HandleWrite()进行写)
        }
    }
}