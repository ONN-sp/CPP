#include "../Base/Logging/logfile.h"
#include "../Base/Logging/logging.h"
#include <string>
#include <unistd.h>

using namespace tiny_muduo;

std::unique_ptr<LogFile> g_logFile;

void outputFunc(const char* msg, int len){
 g_logFile->append(msg, len);
}

void flushFunc(){
  g_logFile->Flush();
}

int main(int argc, char* argv[])
{
  // 日志文件最大200*1000字节，默认线程安全
  g_logFile.reset(new LogFile(nullptr, 200*1000));
  //muduo::Logger::setOutput(outputFunc);
  //muduo::Logger::setFlush(flushFunc);

  // g_logFile 是全局变量，可以直接在lambda闭包中使用
  Logger::SetOutputFunc( [](const char* msg, int len) { 
    g_logFile->append(msg, len); 
  });
  Logger::SetFlushFunc(  []{ g_logFile->Flush(); });

  std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (int i = 0; i < 10000; ++i)
  {
    LOG_INFO << line << i;
    usleep(1000);
  }
}
