#include "EventLoop.h"
#include "TimerQueue.h"
#include "Timestamp.h"
#include <iostream>

EventLoop* g_loop;

void print() { std::cout << "test print()" std::endl;  }

void test()
{
  std::cout << "[test] : test timerQue" std::endl;
  //g_loop->runAfter(1.0, print);
  //g_loop->quit();
}

int main()
{
  EventLoop loop;
  TimerQueue timerQue(&loop);
  Timestamp now(Timestamp::Now());
  timerQue.AddTimer(now, test, 3.0));
  timerQue.AddTimer(now, test, 3.0));
  timerQue.AddTimer(now, test, 5.0));
  /*loop.runAt(times::addTime(TimeStamp::now(), 7.0), test);
  loop.runAt(times::addTime(TimeStamp::now(), 2.0), test);
  loop.runAt(times::addTime(TimeStamp::now(), 5.0), test);*/
  loop.loop();
  return 0;
}
