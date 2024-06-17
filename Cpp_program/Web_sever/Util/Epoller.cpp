#include "Epoller.h"
#include <cassert>
#include <cstring>
#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>
#include <unistd.h>
#include "channel.h"
#include <cerrno>
// #include "logging.h"

using namespace tiny_muduo;

Epoller::Epoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kDefaultEvents) {
  if (epollfd_ == -1) {
    std::cout << "Epoller::Epoller epoll_create1 failed"; //TDOO:modify?
  }
}

Epoller::~Epoller() {
  ::close(epollfd_);
}

Timestamp Epoller::Poll(int timeoutMs, Channels& active_channels) {
  // 调用 epoll_wait 等待事件
  int eventnums = EpollWait(timeoutMs);
  // 填充活跃的 Channel 到 active_channels 容器
  Timestamp now(Timestamp::Now());
  int saveErrno = errno;
  if(eventnums>0)
    FillActiveChannels(eventnums, active_channels);
  else if(eventnums==0)
    std::cout << "timeout" << std::endl;//TODO:modify?
  else{
    if(saveErrno != EINTR){
        errno = saveErrno;
        std::cout << "Epoller::poll" << std::endl;//TODO:modify?
    }
  }
}

int Epoller::EpollWait(int timeout) {
     return ::epoll_wait(epollfd_, &*events_.begin(), 
                                      static_cast<int>(events_.size()), timeout); 
}

void Epoller::FillActiveChannels(int eventnums, Channels& active_channels) {
  // 遍历events_中的事件,将活跃的Channel对象填充到active_channels容器中
  for (int i = 0; i < eventnums; ++i) {
    Channel* ptr = static_cast<Channel*>(events_[i].data.ptr);//注册channel到epoll上   将Channel对象放在epoll_event中的用户数据区,这样可以实现文件描述符和Channel的一一对应  当 epoll 返回事件时，我们可以通过 epoll_event 的 data.ptr 字段快速访问到对应的 Channel 对象，而不需要额外的查找操作
    ptr->SetReceiveEvents(events_[i].events);//将就绪的事件设置给recv_events_
    active_channels.emplace_back(ptr);//将指向这个活跃的channel放入channels中
  }
  // 如果事件数组已满，则将其大小加倍   扩容操作
  if (eventnums == static_cast<int>(events_.size())) {
    events_.resize(eventnums * 2);
  }
}

void Epoller::RemoveChannel(Channel* channel) {
  int fd = channel->fd();
  auto state = channel->state();
  assert(state == ChannelState::kDeleted || state == ChannelState::kAdded);
  // 如果 Channel 处于已添加状态，则将其从 epoll 实例中删除
  if (state == ChannelState::kAdded) {// 从epoll树上中删除,只能是kAdded状态
    UpdateChannel(EPOLL_CTL_DEL, channel);
  }
  // 将 Channel 的状态设置为新建状态，并从 channels_ 容器中移除
  channel->SetChannelState(ChannelState::kNew);
  channels_.erase(fd);//哈希表中也要删
}

void Epoller::UpdateChannel(Channel* channel) {
  int op = 0;
  int events = channel->events();//获取Channel当前感兴趣的事件
  auto state = channel->state(); 
  int fd = channel->fd();
  // 如果 Channel 是新的或者已经被标记为删除的
  if (state == ChannelState::kNew || state == ChannelState::kDeleted) {
    if (state == ChannelState::kNew) {     
      assert(channels_.find(fd) == channels_.end()); // 确保 Channel 的文件描述符不在 channels_ 容器中
      channels_[fd] = channel;
    } 
    else 
    {  // Channel 状态是已删除的  状态已删除不代表Channel的文件描述符已经删除了
      assert(channels_.find(fd) != channels_.end());//确保 Channel 的文件描述符在 channels_ 容器中
      assert(channels_[fd] == channel);// 确保找到的 Channel 与传入的相同,即确保哈希表的映射关系正确
    }
    // 将操作设为添加，并更新 Channel 状态为已添加
    op = EPOLL_CTL_ADD;
    channel->SetChannelState(ChannelState::kAdded);
  } 
  else 
  {
    // 确保 channel 在 channels_ 中
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    // 如果 Channel 没有感兴趣的事件，将操作设为删除，并更新 Channel 状态为已删除
    if (events == 0) {
      op = EPOLL_CTL_DEL;
      channel->SetChannelState(ChannelState::kDeleted); 
    } 
    else 
      // 否则将操作设为修改  更新epoll实例对这个文件描述符的监控事件集
      op = EPOLL_CTL_MOD;
  }
  // 更新 Channel 到 epoll 实例
  Update(op, channel);
}

void Epoller::Update(int operation, Channel* channel){
  struct epoll_event event;
  std::memset(&event, 0, sizeof(event));
  event.events = channel->events();
  event.data.ptr = static_cast<void*>(channel);
  // 调用 epoll_ctl 执行操作，如果失败则记录错误
  if (epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0) {
    std::cout << "Epoller::UpdateChannel epoll_ctl failed";//TDDO:modify? 
  }
}
