#include "Logfile.h"
#include "FileUtil.h"
using namespace tiny_muduo;

LogFile::LogFile(const std::string& filepath, long rollSize) : 
        file_(new FileUtil(filepath)),
        rollSize_(rollSize),
        last_write_(0),
        lastRoll_(0),
        startOfPeriod_(0),
        last_flush_(0){     
    }
LogFile::~LogFile(){
    Flush();
}

void LogFile::Flush() {
    if(mutex_){//!!!   必须先判断,不然程序有bug   这个mutex_可能在其它地方被销毁了,而此时还没有new一个logfile,那么此时就会出现卡死的bug(因为mutex-已经被销毁了)
        MutexLockGuard lock(*mutex_);
        file_->flush(); //不是写入操作,其实可以不用保护线程安全
    }
	else
		file_->flush();
}

void LogFile::append(const char* data, int len){
	if(mutex_){
        MutexLockGuard lock(*mutex_); // 使用互斥锁保证线程安全  这个锁的临界区是整个向日志缓冲区写日志的过程
        append_unlocked(data, len);
	}
	else
		append_unlocked(data, len);
}

// 写入函数
void LogFile::append_unlocked(const char* data, int len) {
  if (file_) {  
    file_->append(data, len); // 写入数据  其实是写入内部缓冲区的,然后flush到本地文件中
	time_t now = ::time(NULL); // 记录最后写入时间
    if(len!=0){
        last_write_ = now;
        written_bytes_ += len;//// !!! 自己在这出错(忘记写了)  调了一天
    }
    if(written_bytes_ > rollSize_)// 写满就日志滚动
        rollFile();
    // 决定是否需要刷新缓冲区
    time_t thisPeriod = last_write_ / kRollPerSeconds_ * kRollPerSeconds_;
    if(thisPeriod != startOfPeriod_)//时间周期数(时间天数)不等    代表进入了新的一天
        rollFile();
    if (last_write_ - last_flush_ > kFlushInterval) {//其实就是当前时间-最后一次的刷新时间  > 刷新时间间隔
      Flush(); // 刷新缓冲区
      last_flush_ = last_write_; // 记录刷新时间
    }
  }
}

//日志记录时间超过当天就进行日志滚动(日志滚动:重新生成日志文件,再向里面写数据)
bool LogFile::rollFile(){
    time_t now = 0;
    std::string filename = getLogFileName();
    // 计算当前时间的滚动周期开始时间
    auto start_of_period = now / kRollPerSeconds_ * kRollPerSeconds_;
    // 比较当前时间是否大于最后一次滚动时间，以决定是否需要滚动日志文件
    if (now > lastRoll_) {
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

long LogFile::writebytes(){
    return written_bytes_;
}






