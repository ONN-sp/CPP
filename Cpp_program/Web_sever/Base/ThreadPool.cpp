#include "ThreadPool.h"
#include <cassert>
#include <iostream>

using namespace tiny_muduo;

ThreadPool::ThreadPool(const std::string& nameArg)
    : mutex_(),
      notEmpty_(mutex_),
      notFull_(mutex_),
      name_(nameArg),
      maxQueueSize_(0),// 初始化任务队列最大长度为0(表示无限制)
      running_(false)
      {}

ThreadPool::~ThreadPool(){
  if (running_)
    stop(); // 如果线程池仍在运行，则停止线程池
}

// 设置任务队列的最大长度 只设置值,不去设置队列
void ThreadPool::setMaxQueueSize(int maxSize) { 
  maxQueueSize_ = maxSize; 
}

// 设置线程初始化回调函数
void ThreadPool::setThreadInitCallback(const Task& cf){ 
  threadInitCallback_ = cf; 
}

// 启动线程池，创建指定数量的线程
void ThreadPool::start(int numThreads)
{
  assert(threads_.empty()); // 断言线程池为空
  running_ = true; // 设置运行状态为true
  threads_.reserve(numThreads); // 预留线程列表的空间
  for (int i = 0; i < numThreads; i++)
  {
    char id[32];
    snprintf(id, sizeof(id), "%d", i+1);
    threads_.emplace_back(new Thread(
          [this]{ this->runInThread(); }, name_+id)); // 创建新线程并将其加入线程列表
    threads_.back()->start(); // 启动新创建的线程
  }
  if (numThreads == 0 && threadInitCallback_)
    threadInitCallback_(); // 如果线程数为0且设置了初始化回调函数，则调用该回调函数
}

// 停止线程池，等待所有线程结束
void ThreadPool::stop()
{
  {
    MutexLockGuard lock(mutex_);
    running_ = false; // 设置运行状态为false
    notEmpty_.NotifyAll(); // 唤醒所有等待非空条件的线程
    notFull_.NotifyAll(); // 唤醒所有等待非满条件的线程
  }
  for (auto& thr : threads_)
    thr->join(); // 等待所有线程结束
}

// 返回当前任务队列的大小(任务数)
int ThreadPool::queueSize() const{
  MutexLockGuard lock(mutex_);
  return queue_.size(); // 返回当前任务队列的大小
}

// 向线程池提交任务
void ThreadPool::run(Task task)
{
  if (threads_.empty())
    task(); // 如果线程池为空，直接执行任务
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull() && running_)// 任务队列已满且线程池在运行的状态
      notFull_.wait(); // 等待任务队列不满
    if (!running_) // 没在运行状态就直接退出
      return;
    assert(!isFull());
    queue_.push_back(std::move(task)); // 将任务加入任务队列(只加入不执行)
    notEmpty_.notify(); // 唤醒等待非空条件的线程
  }
}

// 从任务队列中取出一个任务  这里只取任务,不执行
Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);
  while (queue_.empty() && running_)
    notEmpty_.Wait(); // 等待任务队列不为空
  Task task;
  if (!queue_.empty())
  {
    task = std::move(queue_.front()); // 取出任务队列的第一个任务
    queue_.pop_front(); // 移除任务队列的第一个任务
    if (maxQueueSize_ > 0)
      notFull_.Notify(); // 唤醒等待非满条件的线程
  }
  return task; // 返回取出的任务
}

// 判断任务队列是否已满
bool ThreadPool::isFull() const{
  mutex_.assertLocked(); // 断言当前线程已经锁定互斥锁
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_; // 判断任务队列是否已满
}

// 线程执行的主函数,从任务队列中取出任务执行
void ThreadPool::runInThread()
{
  try
  {
    if (threadInitCallback_)
      threadInitCallback_(); // 执行线程初始化回调函数
    while (running_)
    {
      Task task(take()); // 取出任务队列中的任务
      if (task)
        task(); // 执行任务
    }
  }
  catch (...)
  {
    std::cerr << "unknown exception caught in ThreadPool " << name_ << std::endl;
  }
}
