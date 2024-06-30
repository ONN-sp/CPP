#include "../Base/Logging/asynclogging.h"
#include "../Base/Logging/logging.h"
#include "../Base/Timestamp.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace tiny_muduo;

long kRollSize = 500*1000*1000;

AsyncLogging* g_asyncLog = nullptr;

void asyncOutput(const char* msg, int len)
{
  g_asyncLog->Append(msg, len);
}

void bench(bool longLog)
{
  Logger::SetOutputFunc(asyncOutput);
  int cnt = 0;
  const int kBatch = 500;
  for (int t = 0; t < 30; ++t)
  {
    Timestamp start = Timestamp::Now();
    for (int i = 0; i < kBatch; ++i)
    {
      LOG_INFO << "Hello 0123456789 " << cnt;
	  cnt++;
	  sleep(0.2);//故意睡眠(需要注意:logfile中设置了3秒就往本地文件写日志,因此这里故意睡眠,就会在本地生成多个日志文件(3秒一个,同一个业务线程写的多个日志文件之间是连续的)),为了模拟正常业务线程的工作   如果一个业务线程只做往日志线程写日志这个动作,那么可能在本地文件中的日志会丢包(可能有几个日志消息没统计到,所以一般业务线程不会只做日志输出这个动作)
    }
  }
}

int main(int argc, char* argv[])
{
  std::cout << pthread_self() << std::endl;//输出线程ID

  std::string str;
  AsyncLogging log(str, kRollSize);
  log.StartAsyncLogging();
  g_asyncLog = &log;

  bool longLog = argc > 1;
  bench(longLog);
  log.Stop();//仅仅是为了测试       故意手动停止,这样的话可能3秒还没到,日志线程就被停止了
}