#include "logfile.h"
#include "FileUtil.h"
using namespace tiny_muduo;

LogFile::LogFile(const std::string& filepath, long rollSize) : 
        file_(new FileUtil(filepath)),
        rollSize_(rollSize),
        last_write_(Timestamp::Now()),
        lastRoll_(Timestamp::Now()),
        startOfPeriod_(0),
        last_flush_(Timestamp::Now()){     
    }
LogFile::~LogFile(){
    Flush();
}

void LogFile::Flush() {
    if(file_){
        MutexLockGuard lock(mutex_);
        file_->flush(); //不是写入操作,其实可以不用保护线程安全
    }
}

void LogFile::append(const char* data, int len){
        MutexLockGuard lock(mutex_); // 使用互斥锁保证线程安全  这个锁的临界区是整个向日志缓冲区写日志的过程
        append_unlocked(data, len);
}
// 写入函数
void LogFile::append_unlocked(const char* data, int len) {
  if (file_) {  
    file_->append(data, len); // 写入数据  其实是写入内部缓冲区的,然后flush到本地文件中
    if(file_->writtenbytes() > rollSize_)// 写满就日志滚动
        rollFile();
    last_write_ = Timestamp::Now(); // 记录最后写入时间
    // 决定是否需要刷新缓冲区
    std::chrono::seconds now_write(last_write_.seconds());
    std::chrono::seconds last_flush(last_flush_.seconds());
    time_t thisPeriod = Timestamp::Now().seconds() / kRollPerSeconds_ * kRollPerSeconds_;
    if(thisPeriod != startOfPeriod_)//时间周期数(时间天数)不等    代表进入了新的一天
        rollFile();
    if (now_write - last_flush > kFlushInterval) {//其实就是当前时间-最后一次的刷新时间  > 刷新时间间隔
      Flush(); // 刷新缓冲区
      last_flush_ = Timestamp::Now(); // 记录刷新时间
    }
  }
}

//日志记录时间超过当天就进行日志滚动(日志滚动:重新生成日志文件,再向里面写数据)
bool LogFile::rollFile(){
    Timestamp now = Timestamp::Now();
    time_t now_time_t = now.seconds();
    std::string filename = getLogFileName();
    // 计算当前时间的滚动周期开始时间
    auto start_of_period = now_time_t / kRollPerSeconds_ * kRollPerSeconds_;
    // 比较当前时间是否大于最后一次滚动时间，以决定是否需要滚动日志文件
    if (now_time_t > lastRoll_.seconds()) {
        lastRoll_ = now;//Timestamp没有NonCopyAble
        last_flush_ = now;
        // 计算当前时间的滚动周期开始时间
        startOfPeriod_ = start_of_period;  
        // 创建新的日志文件
        file_.reset(new FileUtil(filename));
        return true;
    }
    return false;
}

// 获取新的日志文件名  保存在的是测试程序的Logfiles文件夹下
std::string LogFile::getLogFileName(){
    std::string default_path = std::move("../Logfiles/LogFile_" +
                             Timestamp::Now().ToFormattedDefaultLogString() +
                             ".log");//时间戳+.log
    return default_path;
}






