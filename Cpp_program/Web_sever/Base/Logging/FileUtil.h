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
                : fp_(filename.empty() ? nullptr : std::fopen(filename.c_str(), "ae")){
            }
            ~FileUtil() {::fclose(fp_);};

            void flush(){
                ::fflush(fp_);//将缓冲区中的数据写入本地文件fp_
            }
            void append(const char* data, int len){
                ::fwrite_unlocked(data, 1, len, fp_);//fwrite_unlocked更高效  
            }
            long writtenbytes() const { 
                return written_bytes_; 
            }
        private:
            FILE* fp_;
            long written_bytes_;
    };
}
#endif
