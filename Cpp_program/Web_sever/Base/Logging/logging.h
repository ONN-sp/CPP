#ifndef LOGGING_H
#define LOGGING_H

#include "../NonCopyAble.h" 
#include <string>

namespace tiny_muduo{
class Logger : NonCopyAble{
    public:
        Logger();
        ~Logger();
        static void setLogFileName(std::string fileName) {logFileName_ = fileName;}
    private:
        static std::string logFileName_;
};
}

#endif