#include "../Base/Logging/Asynclogging.h"
#include "../Base/Logging/Logging.h"
#include "../Base/Timestamp.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>

using namespace tiny_muduo;

long kRollSize = 500*1000*1000;

AsyncLogging* g_asyncLog = nullptr;

void asyncOutput(const char* msg, int len)
{
  g_asyncLog->Append(msg, len);
}

void bench()
{
  Logger::SetOutputFunc(asyncOutput);
  int cnt = 0;
  const int kBatch = 500;
  while(1)
  {
    // for (int i = 0; i < kBatch; ++i)
    // {
      LOG_INFO << "Hello 0123456789 " << cnt;
	  cnt++;
	  // struct timespec req;
	  // req.tv_sec = 1;      // 1 秒
	  // req.tv_nsec = 0L;  // 500 毫秒（500,000,000 纳秒）
	  // nanosleep(&req, NULL);
    // }
  }
}

int main()
{
  std::cout << pthread_self() << std::endl;

  std::string str;
  AsyncLogging log(str, kRollSize);
  log.StartAsyncLogging();
  g_asyncLog = &log;

  bench();
}