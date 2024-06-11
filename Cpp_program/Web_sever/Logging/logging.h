#ifndef LOGGING_H
#define LOGGING_H

class Logger{
    public:
        Logger();
        ~Logger();
        static void setLogFileName(std::string fileName) {logFileName_ = fileName;}
    private:
        static std::string logFileName_;
};

#endif