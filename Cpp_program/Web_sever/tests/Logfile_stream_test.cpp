#include "../Base/Logging/logfile.h"
#include "../Base/Logging/logging.h"
#include <string>
#include <iostream>
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
  std::string str;//不提供日志地址  它会写到默认的路径下(即写到测试程序/tests下的Logfiles文件夹中    tests/Logfiles/)  
  g_logFile.reset(new LogFile(str, 200*1000));
  if (!g_logFile) {
        std::cerr << "Failed to initialize LogFile!" << std::endl;
        return -1;
    }

  // g_logFile 是全局变量，可以直接在lambda闭包中使用
  Logger::SetOutputFunc(outputFunc);
  Logger::SetFlushFunc(flushFunc);

  std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  for (int i = 0; i < 10; ++i)
  {
    LOG_INFO << line << i;
    usleep(10);
  }
}
