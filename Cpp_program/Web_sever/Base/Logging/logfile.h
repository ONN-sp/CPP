#ifndef LOGFILE_H
#define LOGFILE_H

#include <unistd.h>
#include <sys/time.h>
#include "../Timestamp.h"
#include <chrono>
#include <memory>

namespace tiny_muduo{
    static const std::chrono::seconds kFlushInterval(3);
    class LogFile{
        public:
            LogFile(const char*);
            ~LogFile();

            void write(const char*, int);
            void Flush();

            int64_t writtenbytes() const;
        private:
            FILE* fp_; // 
            int64_t written_bytes_;
            std::chrono::system_clock::time_point last_write_;
            std::chrono::system_clock::time_point last_flush_;
    };
}
#endif