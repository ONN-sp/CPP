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
4. `C++`的接口设计中,`= 0`用于声明纯虚函数
5. `static_cast<...>`:强制类型转换
6. `::eventfd`:
   ```C++
   int eventfd(unsigned int initval, int flags);//创建一个指定控制标志的eventfd对象,并返回一个文件描述符
   //initval:初始计数器值
   //flags:控制标志,如EFD_NONBLOCK：非阻塞模式;EFD_CLOEXEC：在 exec 系统调用时自动关闭这个文件描述符
   ```
7. 回调函数的调用是上层来调的(如:`Tcpserver`等)
# Muduo库的学习
1. `C++`中可能出现的内存问题:
   * 缓冲区溢出
   * 空指针/野指针
   * 重复释放
   * 内存泄漏
   * 不配对的`new/delete`
   * 内存碎片
2. `muduo`依赖于`boost`库
3. <mark>`Muduo`的设计理念:每个`EventLoop`运行在一个独立的线程中,每个线程负责管理一组I/O事件,这就是`one (event) loop per thread`</mark>
4. 主线程有一个`EventLoop`,负责接受连接(`accept`在主线程);每个工作线程运行一个独立的`EventLoop`,负责处理已经建立连接(主线程中建立了)上的I/O事件
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
9. `TimerId`是定时器的唯一标识符,用于区分不同的定时器.在`Muduo`中,每个定时器都有一个唯一的`TimerId`,它由定时器的指针和定时器的序号两部分组成.定时器的指针用于定位定时器在`TimerQueue`中的位置,而定时器的序号则用于在多个定时器具有相同到期时间时进行排序
10. `std::pair<Timestamp, Timer*>`:为了处理两个到期时间相同的情况,这里使用`std::pair<Timestamp, Timer*>`作为`std::set`的`key`,这样就可以区分到期时间形相同的定时器
11. `TimerQueue`的成员函数只在其所属的I/O线程调用,因此不用加锁.如:借助`EventLoop::RunOneFunc`可以使`TimeQueue::addTimer`变成线程安全的,即无需用锁.办法是`loop_->RunOneFunc([this, timer](){this->AddTimerInLoop(timer);})`,即把定时器实际的工作转移到所属的I/O线程中去了
# std::pair
1. `std::pair`作用是将两个数据组合成一个数据,这两个数据可以是同一类型或不同类型,如:
   ```C++
   eg:
   std::pair<int, std::function<void()>> task;
   ```
# std::set
1. `std::set`是`C++`标准库中的一个容器,专门用于存储不重复的元素,并且这些元素按照特定的顺序自动排序.其底层实现通常使用红黑树或其他平衡二叉树,保证了插入、删除和查找等操作的时间复杂度位`O(logn)`
2. `std::set`在插入新元素时,会自动根据给定的比较规则(默认是`std::less`,即小于运算符)将元素放在正确的位置:
   ```C++
   int main() {
    std::set<int> mySet = {5, 1, 3, 7, 2};

    // 自动排序,输出有序的元素  自动为std::less(升序)
    for (int elem : mySet) {
        std::cout << elem << " ";
    }
    return 0;
   }
   //输出为=>1 2 3 5 7
   ```
3. `.lower_bound()`:查找集合中第一个不小于指定值的元素的位置
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
# `Poller  Channel  EventLoop`
1. `Poller  Channel  EventLoop`是`Muduo`网络库的核心组件,它们共同协作以实现高效的事件驱动编程
   * `Poller`:这是一个抽象基类,用于封装底层的I/O复用机制,如`epoll`.它负责监听多个文件描述符,并在它们有事件发生时通知`EventLoop`
   * `Channel`:它封装了一个文件描述符和该文件描述符的所有感兴趣的事件(读、写、错误等)及其回调函数.它不管理文件描述符的生命周期,仅负责将其事件分发给相应的回调函数
   * `EventLoop`:它是事件驱动系统的核心循环,负责在事件发生时调度相应的`Channel`处理器.它维护了一个`Poller`,并允许一个不断循环的事件处理机制
2. 三者调用关系:
   * `EventLoop` 持有一个 `Poller` 实例,并管理着一组 `Channel`
   * `Poller` 负责实际的 I/O 多路复用操作,它将发生的事件通知 `EventLoop`
   * `Channel` 与具体的文件描述符关联,负责将文件描述符上的事件分发给对应的处理函数
   * `EventLoop` 通过调用 `Poller` 的 `poll` 方法来等待事件的发生,当事件发生时,它会遍历 `Channel` 列表,并调用每个 `Channel` 的回调函数处理事件
3. ![](EventLopp_Poller_Channel.png)
4. 类比`tiny_httpserver`来理解这三者的关系:(以`tiny_httpserver`主线程为例)`EventLoop`就是`tiny_httpserver`中主线程的事件循环(`while(1)`);`Poller`负责底层的事件多路复用,通过调用系统调用如`epoll_wait`来监听文件描述符上的事件;`Channel`封装了文件描述符及其事件处理函数,负责具体的事件处理逻辑(`Channel`就是主线程调用的就绪事件的对应的执行函数(如:`acceptConn`))
5. 一个线程<=>一个`EventLoop`,一个`EventLoop`可能涉及多个文件描述符
# Channel
1. 活跃的`Channel`<=>这个`Channel`绑定的文件描述符有就绪事件
2. 一个`Channel`<=>一个文件描述符
3. 每个`Channel`对象自始至终只负责一个文件描述符的I/O事件的分发,但它不拥有这个文件描述符,也不会在析构时关闭这个文件描述符.每个`Channel`对象自始至终只属于一个`EventLoop`
4. 假设`epoll`中监听到有4个文件描述符有事件发生,那么我们就把这4个文件描述符所对应的`Channel`称为活跃的
5. 底层向上的调用关系:`Channel::Update()`->`EventLoop::UpdateChannel()`->`Poller::UpdateChannel()`->`epoller::UpdateChannel()`
6. 当`Channel`被标记为`kDeleted`,通常意味着已经执行了`epoll_ctl`的`EPOLL_CTL_DEL`操作.`kDeleted`意味着该`Channel`已经从`epoll`树上删除了
7. `Channel`的三个状态:
   * `kNew`:`Channel`刚创建,尚未加入到`epoll`树上.这通常是默认状态,表示`Channel`尚未参与到任何事件监控中
   * `kAdded`:`Channel`已经被添加到`epoll`树上,并正在监控其感兴趣的事件
   * `kDeleted`:`Channel`曾经在`epoll`树上,但当前已被标记为删除.`Channel`已经从`epoll`树中删除了
# Poller
1. 这是一个基类,因为在muduo中同时支持`poll()`和`epoll()`.本项目只用了`epoll()`,所以只有`epoller`继承了`Poller`,继承后需要重写纯虚函数
2. `Poller`是I/O多路复用的封装,它是`EventLoop`的组成,与`EventLoop`的生命期相同,为`EventLoop`提供`poll()`方法.`poll()`方法是`Poller`的核心功能,它调用`epoll_wait()`获得当前就绪事件,然后注册一个`Channel`与该就绪事件的`fd`绑定(将`Channel`对象放在`epoll_event`中的用户数据区,这样可以实现文件描述符和`Channel`的一一对应.当`epoll`返回事件时,我们可以通过 `epoll_event`的`data.ptr`字段快速访问到对应的`Channel`对象,而不需要额外的查找操作),然后将该`Channel`填充到`active_channels`
3. 在`UpdateChannel()`中,为什么状态为`kDeleted`的`Channel`要被再次添加?
   ```s
   1. 如果 Channel 之前被标记为删除 (kDeleted)，但现在有新的事件需要处理，这说明它需要重新加入到 epoll 中
   2. 使用 EPOLL_CTL_ADD 再次添加 Channel，可以方便地将其重新激活，而不必频繁地删除和重新添加
   ```
4. `Poller.h`中只需要`Channel`类的指针或引用,不需要其完整定义,因此不需要包含`Channel.h`头文件
# EventLoop
1. `EventLoop`是I/O线程的事件循环,它能确保所有注册的事件都在`EventLoop`对象所在的线程中执行
2. `EventLoop::loop()`它调用`Poller::poll()`获得当前活动事件(就绪事件)的`Channel`列表,然后依次调用每个`Channel`的`handleEvent()`函数
3. `EventLoop`是`Channel`和`Poller`的桥梁
4. `EventLoop`<=>`Reactor`
5. 主`EventLoop(Reactor)`负责接收连接请求的到来,然后对应`accept`返回的文件描述符分发给子`EventLoop(Reactor)`,子`EventLoop(Reactor)`中对相应的事件进行回调
6. 假设一种情形:子`EventLoop`被阻塞了(因为`epoll_wait()`没有就绪事件发生),但是此时主`EventLoop`需要向子`EventLoop`添加一个需要理解执行的任务,所以此时必须唤醒这个子`EventLoop`
7. 调用`wakeup()`会获得一个文件描述符(`eventfd`),文件描述符就伴随一个`Channel`.既然子`EventLoop`被阻塞了,那么需要一个就绪事件让它重新唤醒,<mark>本项目采用的是利用主动`wirte`来实现`wakeup`的,此时会伴随一个读事件回调</mark>
8. <mark>`EventLoop`中的`wakeup()`非常经典</mark>
9. <span style="color:red;">`wakeup`:如果不唤醒在`doPendingFunctors()`执行完n个回调后,如果又来了m个回调,但是现在会阻塞在`epoller_->Poll(KPollTimeMs, active_channels_);`中(它里面有`epoll_wait`),所以必须`wakeup()`.总之,`wakeup()`的目的就是为了使新加进来的回调能被立即执行:</span>
    ```C++
    void EventLoop::Loop() {
    assert(IsInThreadLoop()); // 确保当前线程是事件循环所在的线程
    running_ = true; // 标志事件循环开始运行
    quit_ = false;// 是否退出loop
    while (!quit_) {
        active_channels_.clear(); // 清空活跃 Channel 列表
        epoller_->Poll(KPollTimeMs, active_channels_); // 调用 Epoller 的 Poll 函数获取活跃的 Channel 列表
        for (const auto& channel : active_channels_)
            channel->HandleEvent(); //Poller监听哪些Channel发生了事件,然后上报给EventLoop,EventLoop再通知channel处理相应的事件
        doPendingFunctors(); // 处理待回调函数列表中的任务
    }
    running_ = false; // 事件循环结束
    }
    ```
10. `pendingFunctors_`是每个`EventLoop`对象独立存储的,但在多线程服务器中,其它线程(上层)可能需要向当前这个`EventLoop`添加回调函数(如:上层`EventLoop`会写入定时器回调函数),因此`pendingFunctors_`需要线程安全
11. 为什么`if (!IsInThreadLoop() || calling_functors_)`后才`wakeup()`?
    * 首先,其它线程向所属当前线程的`EventLoop`添加任务,那么此时一定要`wakeup`(因为其它线程加进来的任务一定不在当前线程的`EventLoop`中,而当前`EventLoop`可能被阻塞住了);其次,如果新加进来的任务不是已经被执行了的,即此时加进来的是下一轮要去执行的,那么需要`wakeup`(为了使新加进来的任务在下一轮能被立即执行),即`calling_functors_`(当前`pendingFunctors_`执行完毕时会给`calling_functors_`置`false`)
12. `doPendingFunctors`中的经典操作:
    ```C++
    {
        MutexLockGuard lock(mutex_); // 加锁保护待执行任务列表
        functors.swap(pendingFunctors_); // 将待执行任务列表交换到局部变量中  这种交换方式减少了锁的临界区范围(这种方式把pendingFunctors_放在了当前线程的栈空间中,此时这个Functors局部变量就线程安全了),提升了效率,同时避免了死锁(因为Functor可能再调用queueInLoop)
    }
    ```
   `EventLoop::doPendingFunctors()`不是简单地在临界区内依次调用`Functor`,而是把回调列表`pendingFunctors_`交换到局部变量`functors`中去了,这样`functors`对于此`EventLoop`对应的线程它就是其独立拥有的,即线程安全的了(只是多消耗了栈空间).这样操作一方面减小了临界区的长度,另一方面也避免了死锁(此时可能出现的死锁情况:(`C++`中,`std::mutex`表示的就是非可重入互斥锁,这种锁不允许同一线程在已经持有锁的情况下再次获取同一个锁,否则会导致死锁),`Functor`可能再调用`QueueOneFunc`就可能出现一个线程再获取已经持有的锁)
# CurrentThread
1. `__thread`是一种用于声明线程局部存储变量的关键字.在多线程编程中,它允许每个线程拥有自己独立的变量副本,这意味着每个线程一进来后都会拥有这个变量,但是它们之间是互不干扰的
2. `::syscall(SYS_gettid)`:在`Linux`系统中用来获取当前线程ID的系统函数
3. 内联函数在`C++`中是一种特殊的函数,它通过编译器在调用点直接展开函数体,而不是像普通函数那样生成函数调用.这种特性通常用于简单且频繁调用的函数,以提高程序的执行效率和性能









