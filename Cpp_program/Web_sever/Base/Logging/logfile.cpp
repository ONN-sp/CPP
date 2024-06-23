#include "logfile.h"

using namespace tiny_muduo;

LogFile::LogFile(const char* filepath = nullptr) : 
        fp_(::fopen(filepath, "ae")),
        written_bytes_(0),
        last_write_(std::chrono::system_clock::now()),
        last_flush_(std::chrono::system_clock::now()){
        if(!fp_){// 如果没有提供文件路径，则使用默认路径   
            std::string DefaultPath = std::move("./LogFiles/LogFile_" +
                        Timestamp::Now().Timestamp::ToFormattedDefaultLogString() +
                        ".log");
            fp_ = ::fopen(DefaultPath.data(), "ae");
        }
    }
LogFile::~LogFile(){
    Flush();
    fclose(fp_);
}

void LogFile::Flush() { 
    fflush(fp_); 
    last_flush_ = std::chrono::system_clock::now();//更新最新刷新时间
}

int64_t LogFile::writtenbytes() const { 
    last_write_ = std::chrono::system_clock::now();//更新最新写的时间
    return written_bytes_; 
}



