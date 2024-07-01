#ifndef LOGFILE_H
#define LOGFILE_H

#include <unistd.h>
#include <sys/time.h>
#include "../Timestamp.h"
#include <chrono>
#include <memory>
#include "../MutexLock.h"

namespace tiny_muduo{
    static const time_t  kFlushInterval = 3;//定期3秒将缓冲区内的日志flush到硬盘
    class FileUtil;
    class LogFile{
        public:
            LogFile(const std::string& filepath, long rollSize);
            ~LogFile();

            void append(const char*, int);
            void append_unlocked(const char*, int);
            void Flush();
            bool rollFile();//滚动日志  写满换下一个文件+每天零点新建日志文件
            std::string getLogFileName();
            long writebytes();
        private:
            std::unique_ptr<FileUtil> file_; // 日志文件  负责管理 FILE* 类型的文件指针，并在 unique_ptr 析构时自动调用 fclose 函数来关闭文件
            time_t last_write_;
            time_t last_flush_;
            time_t lastRoll_;
            time_t startOfPeriod_;//时间周期数,即当前距离1970年1月1日的的天数
            int64_t written_bytes_;
            const long rollSize_;
            const static int kRollPerSeconds_ = 60*60*24;//一天
            std::unique_ptr<MutexLock> mutex_; // 用于线程安全的写操作
    };
}
#endif