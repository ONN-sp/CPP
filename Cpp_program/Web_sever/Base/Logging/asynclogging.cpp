#include "asynclogging.h"
#include <functional>

using namespace tiny_muduo;

AsyncLogging::AsyncLogging(const std::string& filepath, long rollSize) 
      : running_(false),
        filepath_(filepath),
        rollSize_(rollSize),
        mutex_(),
        cond_(mutex_),
        buffers_(),
        latch_(1),
        thread_(std::bind(&AsyncLogging::ThreadFunc, this), "AsyncLogThread"),
        current_(new Buffer()),// 分配业务线程当前缓冲区
        next_(new Buffer()) {// 分配业务线程备用缓冲区
          buffers_.reserve(16);//为向量预先分配足够的内存以容纳 16 个元素，从而减少内存重新分配的次数
  }
// 析构函数，确保异步日志在销毁前停止
AsyncLogging::~AsyncLogging() {
    if (running_) {
      Stop();
    } 
}
// 停止异步日志记录线程
void AsyncLogging::Stop(){
    running_ = false;
    cond_.Notify();
    thread_.Join();
}
// 开始异步日志记录
void AsyncLogging::StartAsyncLogging(){
    running_ = true;
    thread_.StartThread();
    latch_.wait();
}

void AsyncLogging::Append(const char* data, int len) {
  MutexLockGuard guard(mutex_);
  if (current_->writeablebytes() >= len) // 如果当前缓冲区有足够的空间，直接将数据追加到当前缓冲区
    current_->Append(data, len);// 调用的是logstream的Append方法
  else 
  {
    buffers_.emplace_back(std::move(current_)); // 将当前缓冲区移入后端线程的待写入队列
    if (next_) 
      current_ = std::move(next_); // 使用备用缓冲区作为新的当前缓冲区
    else 
      current_.reset(new Buffer());// 如果没有备用缓冲区，分配一个新的缓冲区(这是用来处理磁盘IO很慢,而前端两个缓冲区很快被写完的情形)
    current_->Append(data, len);
    cond_.Notify();// 当前缓冲区没有足够空间写,即写满    则通知后端线程有新的缓冲区需要写入
  }
}

void AsyncLogging::AsyncFlush(LogFilePtr log) {
    log->Flush();
}

//后端线程执行的主函数
void AsyncLogging::ThreadFunc(){
    latch_.CountDown();// 减少 latch_ 的计数，表示线程已经启动
    //下面是后端线程的两个缓冲区
    BufferPtr newBuffer_current(new Buffer());//newBuffer_current是一个智能指针,所以它指向的内存资源会被自动释放,不用去管
    BufferPtr newBuffer_next(new Buffer());
    LogFilePtr log(new LogFile(filepath_, rollSize_));//log对象离开作用域就会调用LogFile的析构函数(完成flush,因此不需要在asynclogging的析构函数中显示调用AsyncFlush)
    //缓冲区在启动的时候会被初始化填充为0
    newBuffer_current->SetBufferZero();// 初始化新的当前缓冲区
    newBuffer_next->SetBufferZero();// 初始化新的备用缓冲区
    BufferVector buffersToWrite;// 临时缓冲区队列，用于交换(暂存前端线程的buffer队列)
    buffersToWrite.reserve(16);
    while (running_) {
        {
        MutexLockGuard guard(mutex_);
        if (buffers_.empty())// 如果没有缓冲区需要写入
            cond_.WaitForFewSeconds(kBufferWriteTimeout);// 等待一段时间，直到有新的缓冲区或超时(此时会被唤醒)  3秒后肯定会被唤醒
        buffers_.emplace_back(std::move(current_)); // 将前端的当前缓冲区移动到待写入队列中(写满或者达到3秒)
        current_ = std::move(newBuffer_current);// 使用后端的当前缓冲区替换前端的当前缓冲区
        buffersToWrite.swap(buffers_);// 交换缓冲区队列
        if (!next_) // 如果前端的当前缓冲区使用完了(即给它加入前端缓冲区队列了,此时就会把前端的next缓冲区swap到前端的当前缓冲区位置,此时的next==nullptr)
            next_ = std::move(newBuffer_next);
        }    
        for (const auto& buffer : buffersToWrite) // 写入所有需要处理的缓冲区
            log->append(buffer->data(), buffer->len()); // 将缓冲区的数据写入到日志文件中(其实是写到一个文件流中)
        if (log->writebytes() >= kSingleFileMaxSize) // 如果日志文件大小超过了最大限制，创建一个新的日志文件   只有此时才会新创一个.log,其它情况(3秒到了,它只是flush到本地,只要大小没超kSingleFileMaxSize,那么第二个3秒写的还是同一个文件)是不能新建.log的
            log.reset(new LogFile(filepath_, rollSize_));       
        if (buffersToWrite.size() > 2) // 如果有过多的缓冲区，保留两个，多余的丢弃
            buffersToWrite.resize(2);
        if (!newBuffer_current) {// 处理未被占用的缓冲区，准备下一个循环使用
            newBuffer_current = std::move(buffersToWrite.back());// 取出此时最后一个处理结束的缓冲区作为后端新的当前缓冲区
            buffersToWrite.pop_back();// 移除已取出的缓冲区
            newBuffer_current->SetBufferZero();// 重置缓冲区
        }
        if (!newBuffer_next) {
            newBuffer_next = std::move(buffersToWrite.back());// 取出此时最后一个处理结束的缓冲区作为后端新的备用缓冲区
            buffersToWrite.pop_back();
            newBuffer_next->SetBufferZero();
        }
        buffersToWrite.clear();// 清空临时缓冲区队列
    }
}
