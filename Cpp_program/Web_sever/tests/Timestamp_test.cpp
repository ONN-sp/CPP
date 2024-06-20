#include "../Base/Timestamp.h"
#include <vector>
#include <iostream>
#include <array>

using namespace tiny_muduo;

void passByConstReference(const Timestamp& x)
{
  std::cout << x.microseconds() << std::endl;//返回当前微秒数
}

void passByValue(Timestamp x)
{
  std::cout << x.seconds() << std::endl;//返回当前秒数
}

void benchmark()
{
  const int kNumber = 1000 * 1000;
  std::vector<Timestamp> stamps;
  stamps.reserve(kNumber);
  for (int i = 0; i < kNumber; ++i)
    stamps.emplace_back(Timestamp::Now());
  std::cout << stamps.front().ToFormattedString() << std::endl;
  std::cout << stamps.back().ToFormattedString() << std::endl;
  std::array<int, 100> increments = { 0 };
  int64_t start = stamps.front().microseconds();
  for (int i = 1; i < kNumber; ++i)
  {
    int64_t next = stamps[i].microseconds();
    int64_t inc = next - start;
    start = next;
    if (inc < 0)
      std::cout << "reverse!" << std::endl;
    else if (inc < 100)
      ++increments[inc];
    else
      std::cout << "big gap " << inc << std::endl;
  }

  for (int i = 0; i < 100; ++i)
    std::cout << i << ": " << increments[i] << std::endl;
}

int main()
{
  Timestamp now(Timestamp::Now());
  std::cout << now.ToFormattedString() << std::endl;//按%Y%m%d %H:%M:%S格式输出当前时间
  passByValue(now);
  passByConstReference(now);
  benchmark();
}
