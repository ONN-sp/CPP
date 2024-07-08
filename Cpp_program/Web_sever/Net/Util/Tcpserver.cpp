#include "Tcpserver.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Tcpconnection.h"
#include "../../Base/Address.h"
#include "Acceptor.h"
#include "../../Base/Logging/Logging.h"
#include <cassert>

using namespace tiny_muduo;

TcpServer::TcpServer(EventLoop* loop, const Address& address, const std::string& name, Option option)
    : loop_(loop),
      next_conn_id(1),
      name_(name), 
      threads_(std::make_unique<EventLoopThreadPool>(loop, name)),
      acceptor_(std::make_shared<Acceptor>(loop, address, option == Option::kReusePort)),// Acceptor类构造函数需要给定是否重用端口的一个参数
      ip_port_(address.IpPortToString()),
      connection_callback_(),
      message_callback_(){ 
        // 设置新的连接回调函数，使用std::bind绑定TcpServer::HandleNewConnection方法
        acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::HandleNewConnection, this, std::placeholders::_1, std::placeholders::_2));// 占位符用于表示表示将来调用时需要传入的参数位置
      }

TcpServer::~TcpServer(){
    for(auto& item:connections_){
        TcpConnectionPtr ptr(item.second);
        item.second.reset();// 把原始的智能指针复位 后续用ptr代替操作 当ptr出了其作用域 即可释放智能指针指向的对象
        // Listen到连接请求,就会去调用HandleNewConnection()
        ptr->loop()->RunOneFunc(std::bind(&TcpConnection::ConnectionDestructor, ptr));// 将每个连接的销毁操作放入其所属的EventLoop中(放到了pendingFunctors_中)
    }
}
// 设置线程数量  其实就是EventLoopThreadPool中SetThreadNums的上层封装
void TcpServer::SetThreadNums(int numThreads){
    threads_->SetThreadNums(numThreads);
}

// 开启服务器监听
void TcpServer::Start(){
    threads_->StartLoop(threadInit_callback_);
    loop_->RunOneFunc(std::bind(&Acceptor::Listen, acceptor_.get()));// std::bind通常期望传入普通指针或引用,而不是智能指针,所以对于智能指针要用`.get()`获取智能指针内部封装的裸指针
}
// HandleClose()是channel中的关闭回调
void TcpServer::HandleClose(const TcpConnectionPtr& ptr){
    // 当一个TcpConnection关闭时,将关闭操作放入主EventLoop的任务队列中(放到了pendingFunctors_中)
    loop_->RunOneFunc(std::bind(&TcpServer::HandleCloseInLoop, this, ptr));
}
// HandleCloseInLoop不是channel中的关闭回调,它是HandleClose()这个channel关闭回调里的回调函数  这是一个上层回调
void TcpServer::HandleCloseInLoop(const TcpConnectionPtr& ptr){
    // 确保要关闭的连接在哈希表中
    assert(connections_.find(ptr->name())!=connections_.end());
    connections_.erase(ptr->name());
    LOG_INFO << "TcpServer::HandleCloseInLoop - remove connection " << "["  
           << ip_port_ << '#' << ptr->id() << ']';
    EventLoop* loop = ptr->loop();// 获取TcpConnection对象所属的loop
    loop->QueueOneFunc(std::bind(&TcpConnection::ConnectionDestructor, ptr));// 这里必须要放在Loop里,如果直接执行就可能出现TcpConnection的被析构了(它所拥有的channel_也会析构),但此时可能channel_回调的事件并没结束,导致channel_无引用定义
}

void TcpServer:: HandleNewConnection(int connfd, const Address& address){
    // 取一个子EventLoop来管理connfd对应的channel
    EventLoop* sub_loop = threads_->NextLoop();
    // 创建一个新的TcpConnection对象来处理新连接
    TcpConnectionPtr ptr(std::make_shared<TcpConnection>(sub_loop, connfd, next_conn_id));
    // 将新连接保持到哈希表中
    std::string connName = ip_port_ + std::to_string(connfd) + std::to_string(next_conn_id);// 一个TcpConnection名称=ip_port + 文件描述符值+该TcpConnection的ID(next_conn_id)
    connections_[connName] = ptr;
    // 设置连接的各种回调函数
    ptr->SetConnectionCallback(connection_callback_);
    ptr->SetMessageCallback(message_callback_);
    ptr->Set(std::bind(&TcpServer::HandleClose, this, std::placeholders::_1));
    // 打印日志
    LOG_INFO << "TcpServer::HandleNewConnection - new connection " << "[" 
        << ip_port_ << '#' << next_conn_id << ']' << " from " 
        << address.IpPortToString();
    // 更新next_conn_id
    next_conn_id++;
    if(next_conn_id == INT_MAX)
        next_conn_id = 1;
    // 将连接的建立操作放入其所属的EventLoop的任务队列中
    sub_loop->RunOneFunc(std::bind(&TcpConnection::ConnectionEstablished, ptr));
}



