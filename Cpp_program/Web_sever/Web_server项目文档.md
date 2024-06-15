1. `getopt()`:这是一个C标准库函数,用于解析命令行参数,如:`./your_program_name -t 8 -p 8080 -l /var/log/webserver.log`:此时会启动服务器,使用8个线程(`-t 8`),监听端口号为8080(`-p 8080`),并将日志输出到`/var/log/webserver.log`
   ```C++
   int getopt(int argc, char* argv[], const char* optstring);
   //argc:命令行参数的数量,包括程序名称在内
   //argv:一个指向以nullptr结尾的指针数组,其中每个指针指向一个以nullptr结尾的字符串,表示一个命令行参数
   //optstring:一个包含所有有效选项字符的字符串.每个字符代表一个选项,如果后面跟着一个冒号":",则表示该选项需要一个参数.如,字符串"t:l:p"表示有三个选项,分别是't' 'l' 'p',其中't' 'p'后面需要一个参数
   //getopt()当所有选项解析完毕时,返回-1
   ```
2. 对于`optarg`,它是一个全局变量,用于存储当前`getopt()`解析到的选项的参数值.如:如果你的命令行参数是`-t 8`,当程序解析到`-t`选项时,`optarg`将被设置为`8`
3. `int main(int argc, char* argv[])`:程序入口点,其中:
   * `argc`:一个整数,表示传递给程序的命令行参数的数量,它至少为1,因为命令行第一个参数永远是程序的名称(通常是可执行文件的路径)
   * `argv`:一个指向字符指针数组的指针,表示传递给程序的实际命令行参数.`argv[0]`是指向程序名称的字符串,`argv[1]`到`argv[argc-1]`包含了传递给程序的其它参数,如:
   ```C++
    ./my_program arg1 arg2 arg3
    //argv[0]:指向字符串"./my_program"
    //argv[1]:指向字符串 "arg1"
    //argv[2]:指向字符串 "arg2"
    //argv[3]:指向字符串 "arg3"
    ```
    `argc argv`是由操作系统在调用程序时自动生成的参数.当在命令行中运行一个程序时,操作系统会自动将命令行参数传递给该程序
# 定时器
1. 计算机中的时钟不是理想的计时器,它可能会漂移或跳变
2. `Linux`的获取当前时间的函数:
   ```C++
   1. 
   time_t time(time_t* tloc);//返回从1970年1月1日到当前时间的秒数.若tloc!=nullptr,则time()会将当前时间保存到tloc指向的内存位置;若为nullptr,则返回时间值
   //time_t表示时间的基础类型,通常被定义为一个整数类型(__int64),用于存储自197年1月1日00:00:00 UTC起经过的秒数
   2. int ftime(struct timeb* tp);//将当前时间的秒数和毫秒数存储在tp中,比time()更精细
   struct timeb {
    time_t time;       // 从 Unix 纪元开始的秒数
    unsigned short millitm; // 秒后经过的毫秒数
    short timezone;    // 本地时区相对于 UTC 的分钟数偏移
    short dstflag;     // 日光节约时间标志（非零表示处于日光节约时间）
   };
   3. int gettimeofday(struct timeval* tv, struct timezone* tz);//获取当前的系统时间,存储在tv中,这个结构包含秒和微秒的信息
   //struct timezone* tz:通常被弃用,设为nullptr
   struct timeval {
    time_t tv_sec;    // 从 Unix 纪元开始的秒数
    suseconds_t tv_usec; // 秒后经过的微秒数
   };
   4. int clock_gettime(clockid_t clk_id, struct timespec* tp);//能提供纳秒级的时间精度
   struct timespec {
    time_t tv_sec;    // 秒数
    long tv_nsec;     // 纳秒数
   };
   ```
3. `Linux`的定时函数:
   ```C++
   1. unsigned int sleep(unsigned int seconds);//将调用进程挂起指定的秒数
   2. unsigned int alarm(unsigned int seconds);//用于设置一个闹钟，在指定的秒数之后发送 SIGALRM 信号给调用进程。它通常用于定时触发信号处理器
   3. int usleep(useconds_t usec);//将调用进程挂起指定的微秒数
   4. int nanosleep(const struct timespec *req, struct timespec *rem);//将调用进程挂起指定的秒数和纳秒数
   5. int clock_nanosleep(clockid_t clk_id, int flags, const struct timespec *request, struct timespec *remain);//类似于 nanosleep()，但它允许指定不同的时钟源，并且可以选择相对或绝对的时间来休眠
   6. 
   int timerfd_create(int clockid, int flags);//创建一个新的定时器描述符
   //clockfd:指定定时器时钟源.可以是 CLOCK_REALTIME 或 CLOCK_MONOTONIC
   //flags:指定定时器的行为,通常为0
   //成功返回一个文件描述符;若出错返回1
   int timerfd_gettime(int fd, struct itimerspec *curr_value);//用于获取定时器描述符的当前超时设置
   //struct itimerspec *curr_value: 用于存储当前超时设置的 itimerspec 结构体指针
   struct itimerspec {
     struct timespec it_interval;  /* 周期定时器间隔.一旦定时器第一次触发后，it_interval 定义了多长时间后定时器再次触发。如果 it_interval 的值为零，则表示定时器只触发一次，不会重复 */
     struct timespec it_value;     /* 定时器的首次触发时间.当设置定时器时,it_value 定义了多长时间后定时器第一次触发。如果 it_value 的值为零，则表示定时器立即触发 */
   };
   int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);//用于设置定时器文件描述符的超时值
   //flags:指定定时器的行为,通常为0
   //const struct itimerspec *new_value: 指定新的超时值的 itimerspec 结构体指针
   //struct itimerspec *old_value: 如果不为 NULL，则存储之前的超时值
   //返回0表示成功,新的超时值已经设置;返回-1则表示失败
   ```
4. 对于`Linux`的计时和定时函数的选择:`gettimeofday()`和`timerfd_create()/timerfd_gettime()/timerfd_settime()`.原因:
   * `time()`精度低,`ftime()`已被废弃,`clock_gettime()`精度高但开销更大
   * `sleep() alarm() usleep()`在实现时有可能用了`SLGALRM`信号,在多线程编程中处理信号相当棘手
   * `nanosleep() clock_nanosleep()`是线程安全的,但在非阻塞网络编程中,绝对不能用让线程挂起的方式来等待一段时间,这样一来程序会失去响应
   * `timerfd_create()/timerfd_gettime()/timerfd_settime()`把时间变成了一个文件描述符,该"文件"在定时器超时的那一刻变得可读,这样就能方便的融入`select() poll() epoll()`.此定时方法本身不会挂起程序,它是否阻塞取决于`recv()`,因为它可读时就代表定时完成,这有点像时间回调函数
5. UTC(协调世界时)是全球标准的时间尺度,用来同步世界各地的时钟.UTC时间是没有时区的时间标尺,全球的时区偏移量都是基于UTC时间的,如:北京时间为UTC+8,美国东部时间为UTC-5
6. `std::tm`结构体:
   ```C++
   struct tm {
    int tm_sec;   // 秒，范围 0-59
    int tm_min;   // 分，范围 0-59
    int tm_hour;  // 小时，范围 0-23
    int tm_mday;  // 一个月中的第几天，范围 1-31
    int tm_mon;   // 月份，范围 0-11（0 代表一月，1 代表二月，依此类推）
    int tm_year;  // 年份，从 1900 年开始的年数
    int tm_wday;  // 一周中的第几天，范围 0-6（0 代表星期日，1 代表星期一，依此类推）
    int tm_yday;  // 一年中的第几天，范围 0-365
    int tm_isdst; // 夏令时标识，负数表示无效
   };
   ```
7. `C++`中:
   * 拷贝赋值运算符是在两个同类型对象之间进行的赋值操作,如`a=b`,其中`a`和`b`都是同一个类的实例
   * `timer.h`中成员级别的赋值不是拷贝赋值
8. `::close(timerfd_)  ::read`:为了显式地指明我们要调用操作系统提供的`close()  read()`函数,而非当前作用域内的其他可能存在的同名函数,我们使用作用域解析运算符`::`来表示我们要调用全局命名空间中的`close()  read()`函数
# std::pair
1. `std::pair`作用是将两个数据组合成一个数据,这两个数据可以是同一类型或不同类型,如:
   ```C++
   eg:
   std::pair<int, std::function<void()>> task;
   ```
# std::set
1. `.lower_bound()`:查找集合中第一个不小于指定值的元素的位置
#  std::function<void()>
1. 这是一个通用的<mark>函数包装器</mark>,用于存储任意可调用对象(函数、函数指针、Lambda表达式等),并提供一种统一的方式来调用这些可调用对象
2. <span style="color:red;">严格来说`std::function<void()>`:表示不接受任何参数且无返回值的可调用对象,但是它仍然可以用作包装带有参数的函数的包装器:</span>
   ```C++
   1. 使用Lambda表达式忽略参数:
   void foo(int x){
      std::cout << x << std::endl;
   }
   std::function<void()> func = [](){
      foo(42);
   }
   func();
   2. 使用std::bind部分应用参数:
   void foo(int x){
      std::cout << x << std::endl;
   }
   std::function<void()> func = std::bind(foo, 42);
   func();
   3. 使用函数指针
   void foo() {
    std::cout << "Hello, World!" << std::endl;
   }
   std::function<void()> funcPtr = foo;
   funcPtr(); // 调用 foo 函数
   ```









