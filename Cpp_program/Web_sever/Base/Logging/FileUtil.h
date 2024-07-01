#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <string>
#include <memory>
#include <cstdio>

namespace  tiny_muduo
{
    class FileUtil{//一些日志文件的简单操作,这个.h只是为了方便管理
        public:
            FileUtil(std::string filename) 
                : fp_(::fopen(filename.c_str(), "ae")){
                if(!fp_){// 如果没有提供文件路径，则使用默认路径   
                    std::string DefaultPath = std::move("../Logfiles/LogFile_" +
                              Timestamp::Now().Timestamp::ToFormattedDefaultLogString() +
                              ".log");
                    fp_ = ::fopen(DefaultPath.data(), "ae");
                }
            }
            ~FileUtil() {::fclose(fp_);};

            void flush(){
                ::fflush(fp_);//将缓冲区中的数据写入本地文件fp_
            }
            void append(const char* data, int len){
                ::fwrite_unlocked(data, 1, len, fp_);//fwrite_unlocked更高效  

            }
        private:
            FILE* fp_;
    };
}
#endif
