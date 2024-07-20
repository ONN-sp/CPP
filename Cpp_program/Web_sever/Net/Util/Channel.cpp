#include "Channel.h"
#include <sys/poll.h>
#include <sys/epoll.h>
#include "../../Base/Logging/Logging.h"
#include <iostream>
#include <cassert>

using namespace tiny_muduo;

Channel::Channel(EventLoop* loop, const int& fd)
    : loop_(loop),// one loop per thread
      fd_(fd),
      events_(0),
      recv_events_(0),
      tied_(false),
      addedToLoop_(false),
      state_(ChannelState::kNew) {}
      
Channel::~Channel() {
    assert(!addedToLoop_);// 确保当前Channel必须被加进某个loop
    assert(!loop_->hasChannel(this));// 确保Channel在被销毁之前,它先从EventLoop中被移除了
}

//设置读回调函数(移动语义)
void Channel::SetReadCallback(EventCallback&& callback){
    read_callback_ = std::move(callback);
}

//设置读回调函数(拷贝语义  参数传递)
void Channel::SetReadCallback(const EventCallback& callback){
    read_callback_ = callback;
}

//设置写回调函数(移动语义)
void Channel::SetWriteCallback(EventCallback&& callback){
    write_callback_ = std::move(callback);
}

//设置写回调函数(拷贝语义)
void Channel::SetWriteCallback(const EventCallback& callback){
    write_callback_ = callback;
}

//设置错误回调函数(移动语义)
void Channel::SetErrorCallback(EventCallback&& callback){
    error_callback_ = std::move(callback);
}

//设置错误回调函数(拷贝语义)
void Channel::SetErrorCallback(const EventCallback& callback){
    error_callback_ = callback;
}

//设置错误关闭函数(移动语义)
void Channel::SetCloseCallback(EventCallback&& callback){
    close_callback_ = std::move(callback);
}

//设置错误关闭函数(拷贝语义)
void Channel::SetCloseCallback(const EventCallback& callback){
    close_callback_ = callback;
}

//将Channel绑定到某个对象A的生命周期上,避免使用时对象A已被销毁
//Tie什么时候被调用了?   TcpConnection => channel_
/**
 * @brief TcpConnection中注册了Channel对应的回调函数,传入的回调函数均为TcpConnection对象的成员方法,因此可以说明:Channel的结束一定早于TcpConnection对象
 * 此处用Tie去解决TcpConnection和Channel的生命周期时长问题,从而保证了Channel对象能够在TcpConnection销毁前销毁
 */
void Channel::Tie(const std::shared_ptr<void>& ptr){// 防止对象ptr在事件处理过程(它自己的channel中的回调事件)中被销毁
    tied_ = true;
    tie_ = ptr;
}

//启用读事件  Update()就是将events_此时的状态给注册到epoll树上去(epoll_wait)
void Channel::EnableReading(){
    events_ |= (EPOLLIN|EPOLLPRI);
    Update();//调用epoll_ctl
}

//启用写事件
void Channel::EnableWriting(){
    events_ |= EPOLLOUT;
    Update();
}

//禁用所有事件
void Channel::DisableAll(){
    events_ = 0;
    Update();
}

//禁用写事件
void Channel::DisableWriting(){
    events_ &= ~EPOLLOUT;
    Update();
}

//更新事件状态
void Channel::Update(){
    addedToLoop_ = true;
    loop_->UpdateChannel(this);
}

//移除当前Channel
void Channel::Remove(){
    addedToLoop_ = false;
    loop_->RemoveChannel(this);
}

//设置接收到的事件
void Channel::SetReceiveEvents(int events){
    recv_events_ = events;
}

//设置Channel状态
void Channel::SetChannelState(ChannelState state){
    state_ = state;
}

//获取文件描述符
int Channel::fd() const{
    return fd_;
}

//获取事件标志
int Channel::events() const{
    return events_;
}

// 获取接收到的事件标志
int Channel::recv_events() const { 
    return recv_events_; 
}

// 获取 Channel 的状态
ChannelState Channel::state() const {
     return state_; 
}

// 判断是否在监听写事件
bool Channel::IsWriting() const { 
    return events_ & EPOLLOUT; 
}

// 判断是否在监听读事件
bool Channel::IsReading() const { 
    return events_ & (EPOLLIN | EPOLLPRI); 
}

//处理事件
void Channel::HandleEvent(){
    if (tied_){
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
            HandleEventWithGuard();
        //如果不能提升到HandleEventWithGuard(),则不做任何处理,说明Channel的TcpConnection对象已经不存在了
    } 
    else 
        HandleEventWithGuard();
}

//带有保护机制的事件处理   下面四种回调函数是在TcpConnection的构造函数中注册的
void Channel::HandleEventWithGuard(){
    LOG_INFO << "active channel handleEvent revents: " << recv_events_;
    if((recv_events_ & EPOLLHUP) && !(recv_events_ & EPOLLIN))// POLLHUP表示挂起(挂断)事件,表示事件被挂起或关闭(挂起和关闭时都要调用close_callback_)
        if(close_callback_)
            close_callback_();
    if (recv_events_ & POLLNVAL) 
        LOG_ERROR << "Channel::HandleEventWithGuard POLLNVAL";
    if (recv_events_ & (EPOLLERR | POLLNVAL))  //错误事件
        if (error_callback_) 
            error_callback_();       
    if (recv_events_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) //读事件
        if (read_callback_) 
            read_callback_();     
    if (recv_events_ & EPOLLOUT) //写事件
        if (write_callback_)
            write_callback_();      
}


