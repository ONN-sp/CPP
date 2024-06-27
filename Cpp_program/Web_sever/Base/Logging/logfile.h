#ifndef LOGFILE_H
#define LOGFILE_H

#include <unistd.h>
#include <sys/time.h>
#include "../Timestamp.h"
#include <chrono>
#include <memory>
#include "../MutexLock.h"

namespace tiny_muduo{
    static const std::chrono::seconds kFlushInterval(3);//定期3秒将缓冲区内的日志flush到硬盘
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
        private:
            std::unique_ptr<FileUtil> file_; // 日志文件  负责管理 FILE* 类型的文件指针，并在 unique_ptr 析构时自动调用 fclose 函数来关闭文件
            Timestamp last_write_;
            Timestamp last_flush_;
            Timestamp lastRoll_;
            time_t startOfPeriod_;//时间周期数,即当前距离1970年1月1日的的天数
            const long rollSize_;
            const static int kRollPerSeconds_ = 60*60*24;//一天
            MutexLock mutex_;; // 用于线程安全的写操作
    };
}
#endif