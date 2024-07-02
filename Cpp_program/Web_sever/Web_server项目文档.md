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
8. <mark>`muduo  web_server`的具体的`recv send accept`函数的调用是在回调函数内部,回调函数是被是否有对应就绪事件而触发的(最终是在`Channel`中的`HandleEvent`触发的,根据现在的就绪事件类型触发相应的回调函数),而不是先在外部调用`recv send accept`后触发回调函数(如先调用`recv`再触发对应的`Readcallback`).`wakeup_channel_->SetReadCallback([this]{this->HandleRead();})`:`read`函数在`HandleRead()`内部</mark>
9. 本项目中对于基本组件(`mutex condition thread`等)都不是直接在需要使用的地方调用标准库封装好的API,而是先自己将它们的创建、使用等方法封装成一个`RAII`对象,再在其他地方进行调用
10. 本项目有两个3秒的应用地方:
    * 3秒后业务线程的前端缓冲区队列会和日志线程的后端缓冲区队列进行交换(`buffersToWrite.swap(buffers_);`)
    * 3秒后`Logfile`就会把写入`FixedBuffer`中的日志信息写入到本地文件`::fflush(fp_)`
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
5. `muduo`日志库是`C++ stream`风格,这样用起来更自然,不必费心保持格式字符串和参数类型的一致性,可以随用随写,而且是类型安全的
6. `muduo`广泛使用`RAII`这一方法来管理资源的生命周期 
# 定时器
1. 计算机中的时钟不是理想的计时器,它可能会漂移或跳变
2. `Linux`的获取当前时间的函数:
   ```C++
   1. 
   time_t time(time_t* tloc);//返回从1970年1月1日到当前时间的秒数.若tloc!=nullptr,则time()会将当前时间保存到tloc指向的内存位置;若为nullptr,则返回时间值
   //time_t表示时间的基础类型,通常被定义为一个整数类型(__int64),用于存储自1970年1月1日00:00:00 UTC起经过的秒数
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
   * `timerfd_create()/timerfd_gettime()/timerfd_settime()`把时间变成了一个文件描述符,该"文件"在定时器超时的那一刻变得可读(即定时完成时就会触发一个可读事件),这样就能方便的融入`select() poll() epoll()`.此定时方法本身不会挂起程序,它是否阻塞取决于`recv()`,因为它可读时就代表定时完成,这有点像时间回调函数
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
12. 在实际编程任务中,一定不能用`sleep()`或类似的方法来让程序原地停留等待,这会让程序失去响应.对于定时任务,我们应该把它变成一个特定的事件(`timerfd_create()/timerfd_gettime()/timerfd_settime()`将其变成了一个可读事件),到定时时间就会触发一个相应定时完成后需要完成的处理函数(即回调)
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
   * `EventLoop` 通过调用 `Poller` 的 `poll` 方法来等待事件的发生,当获取到活动事件后,它会获取活动事件的`Channel`列表(即注册与当前就绪事件的`fd`对应的`Channel`,把这些活跃的`Channel`放入`active_channels`),再依次调用每个`Channel`的回调函数处理事件
3. ![](EventLopp_Poller_Channel.png)
4. 类比`tiny_httpserver`来理解这三者的关系:(以`tiny_httpserver`主线程为例)`EventLoop`就是`tiny_httpserver`中主线程的事件循环(`while(1)`);`Poller`负责底层的事件多路复用,通过调用系统调用如`epoll_wait`来监听文件描述符上的事件;`Channel`封装了文件描述符及其事件处理函数,负责具体的事件处理逻辑(`Channel`就是主线程调用的就绪事件的对应的执行函数(如:`acceptConn`))
5. 一个线程<=>一个`EventLoop`,一个`EventLoop`可能涉及多个文件描述符
6. `epoll_wait()`:返回的是就绪的文件描述符个数,即就绪事件的个数,就绪事件被放在了`events_`中
# enum与enum class的区别
1. 这是两种不同的枚举类型声明方式:
   * `enum class`引入了作用域,枚举值被限制在枚举类型的作用域内,不会自动转换为整数.因此,枚举值必须显式地指定为枚举类型的成员:
      ```C++
      enum class ChannelState { Open, Closed };
      ChannelState state = ChannelState::Open;
      // 使用枚举值时必须指定作用域
      if (state == ChannelState::Open) { /* ... */ }
      ```
   * `enum`没有引入新的作用域,枚举值可以直接被用作整数.在枚举类型的作用域外部,枚举值可以隐式地转换为整数:
      ```C++
       enum ChannelState { Open, Closed };
      ChannelState state = Open;  // 不需要指定作用域(即不需要ChannelState::)
      // 在作用域外部，枚举值可以隐式转换为整数
      if (state == Open) { /* ... */ }
      ```
   * 使用`enum class`可以提供更好的封装和名称空间隔离,避免命名冲突,并且需要显式地进行枚举值的类型转换
   * 使用传统的`enum`保留了与`C`语言风格枚举相同的行为,不具备封装性和名称空间隔离,并且允许隐式转换为整数
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
7. 调用`wakeup()`会获得一个文件描述符(`::eventfd()`返回的),文件描述符就伴随一个`Channel`.既然子`EventLoop`被阻塞了,那么需要一个就绪事件让它重新唤醒,<mark>本项目采用的是利用主动`wirte`来实现`wakeup`的,此时会伴随一个读事件回调</mark>
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
1. `__thread`是一种用于声明线程局部存储变量的关键字.在多线程编程中,它允许每个线程拥有自己独立的变量副本,这意味着每个线程一进来后都会拥有这个变量,但是它们之间是互不干扰的.`__thread`可以用来修饰那些带有全局性且值可能变,但是又不值得用全局变量保护的变量
2. `::syscall(SYS_gettid)`:在`Linux`系统中用来获取当前线程ID的系统函数
3. 内联函数在`C++`中是一种特殊的函数,它通过编译器在调用点直接展开函数体,而不是像普通函数那样生成函数调用.这种特性通常用于简单且频繁调用的函数,以提高程序的执行效率和性能
4. `__thread`使用规则:只能用于修饰`POD`类型(包括基本数据类型`int float char enum`等、结构体等,不能有自定义的构造函数、析构函数、拷贝构造函数、移动构造函数、虚函数、赋值运算符等),不能修饰`class`类型,因为无法自动调用构造函数和析构函数.`__thread`可以用于修饰全局变量、函数内的静态变量,但是不能用于修饰函数的局部变量或`class`的普通成员变量.`__thread`初始化只能用编译器常量:
   ```C++
   1.
   __thread int counter;  // 基本类型
   2.
   struct NonPOD1 {
    int x;
    NonPOD1() : x(0) {}  // 自定义构造函数，不符合要求
   };
   struct NonPOD2 {
      virtual void func() {}  // 包含虚函数，不符合要求
   };
   ```
5. 编译常量:指的是在编译期就能确定其值的常量,包含:`字面值常量`:`42 3.14 'a'`等;`const常量`;`枚举类型`;`constexpr`;`模板参数`:
   ```C++
   1.
   const int maxCount = 100;
   const double pi = 3.14159;
   2.
   enum Color { Red = 1, Green = 2, Blue = 3 };
   3.
   constexpr int arraySize = 128;
   constexpr double square(double x) { return x * x; }
   4.
   template<int N>
   struct Factorial {
      static const int value = N * Factorial<N - 1>::value;
   };
   template<>
   struct Factorial<0> {
      static const int value = 1;
   };
   ```
6. `constexpr`:用于声明可以在编译时求值的常量表达式,它提供了比`const`更强大的编译时的计算能力:
   * 与`const`类似,`constexpr`可以用于定义在编译期求值的常量变量
   * `constexpr`关键字可以修饰函数,表示这个函数可以在编译期求值
   * 与`const`不同,`constexpr`提供了在编译期进行复杂计算的能力,包括递归和逻辑运算
   ```C++
   constexpr int arraySize = 128;
   constexpr int factorial(int n) {
      return (n <= 1) ? 1 : (n * factorial(n - 1));
   }
   constexpr int result = factorial(5); // factorial 函数是一个 constexpr 函数，如果传递给它的参数是一个编译时常量（如 5），那么它的结果也是一个编译时常量
   ```
# Logging
## logstream
1. `logstream.cpp logstream.h`:类比于`ostream.h`类,它们主要是重载了`<<`(本项目的日志库采用与`muduo`一样的`C++ stream`),此时`logstream`类就相当于`std::cout`,能用`<<`符号接收输入,`cout`是输出到终端,而`logstream`类是把输出保存到自己内部的缓冲区,可以让外部程序把缓冲区的内容重定向输出到不同目标,如:文件(对于日志输出,就是输出到本地文件中)、终端、`socket`(即`logstream`流对象是输出到`buffer (Buffer = FixedBuffer<kSmallSize>`)`的)
2. <mark>`logstream`类使用了自定义的`FixedBuffer`类,通过预先分配的固定大小的缓冲区来存储日志数据(注意:`logstream.cpp logstream.h`并没有给出日志输出到哪个本地文件啥的,只是到一个预先分配的缓冲区里了),这样可以减少频繁的动态内存分配,提高性能</mark>
3. `logStream`类里面有一个`Buffer`(`Buffer = FixedBuffer<kSmallSize>`)成员(就是`FixedBuffer`类的,不是`tiny_muduo::Buffer`类).该类主要负责将要记录的日志内容放到这个`Buffer`里面.包括字符串,整型、`double`类型(整型和`double`要先将之转换成字符型(`FormatInteger`和`snprintf(buf, sizeof(buf), "%g", num)`),再放到`buffer`里面).该类对这些类型都重载了`<<`操作符.这个`logStream`类不做具体的IO操作(`::recv ::send`)
4. <mark>`Buffer = FixedBuffer<kSmallSize>`定义的`Buffer`相当高效,因为它是利用`memcpy()`复制数据的,而`gcc`编译器会直接将`memcpy()`展开为内联代码,提高了执行效率</mark>
5. <mark>`gcc`内置函数不是`C`标准定义的,而是由编译器自定义的.内置函数(`memcpy() strrchr()`等)都是`inline`的</mark>
6. 流对象的形象理解:想象水流通过一根管道,水代表数据,管道代表流对象.数据在管道中流动,无论水的来源(例如水龙头、湖泊还是去向(例如水槽、河流),你都可以通过管道来运输水.在`C++`中,`std::cin`就像从键盘输入的管道,而`std::cout`就像输出到控制台的管道.对于`logstream`中的流对象,就是输出到`Buffer = FixedBuffer<kSmallSize>`这样的`buffer`中的管道
## logfile
1. `logfile`类是`webserver`日志库后端的实际管理者,主要负责日志的滚动
2. 为了在写日志的时候更高效,写日志到缓冲中使用非加锁的`::fwrite_unlocked`函数,因此要求外部单线程处理或加锁,保证缓冲区日志不会发生交织混乱
3. `fwrite_unlocked`比`fwrite`更高效
4. 对于日志滚动,其实就是重新生成日志文件,再向里面写数据;本项目有两种情况日志滚动:
   * 当日志消息文件大小达到设定值`rollSize_`就滚动
   * 当到了新的一天,日志也会滚动
5. 日志库不能每条消息都`flush`硬盘,更不能每条日志都`open/close`,这样性能开销太大.本项目定期(默认3秒)将缓冲区日志消息`flush`到硬盘(注意:这里的3秒指的是写入磁盘的时间间隔,而`asynclogging`中的3秒指的是前端和后端线程交换的时间间隔)
6. 日志信息->`::fwirte_unlocked`写到`fp_`所代表的文件流对应的内部缓冲区中->`flush`到本地文件中
7. 日志都在`./LogFiles/`文件夹中
# logging
1. 日志信息是有等级的,由低到高为:`DEBUG INFO WARN ERROR FATAL`:
   ```C++
   enum class Level{
      DEBUG, // 调试信息级别
      INFO,  // 普通信息级别
      WARN,  // 警告信息级别
      ERROR, // 错误信息级别
      FATAL  // 致命错误信息级别
   };

   #define LOG_INFO \
   if (LogLevel() <= tiny_muduo::Logger::Level::INFO) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::Level::INFO).stream()
    //当输出的日志级别高于语句的日志级别(INFO),则打印日志(打印到屏幕)是个空操作,运行时开销接近0
    ```
2. 本项目的日志库的消息格式为:
   ```s
   日期 时间 微秒 线程ID 级别 写入的日志正文消息 - 源文件名:行号
   ```
   ![](日志消息.png)
3. 本项目的默认路径:`./webserver/tests/Logfiles/`
4. 本项目的日志文件名:`LogFile_当前时间_微秒.log`
5. 本项目的日志的使用方式(非异步):
   ```C++
   LOG_INFO << "AAA";
   //LOG_INFO是一个宏，展开后为：muduo::Logger(__FILE__, __LINE__).stream() << "AAA";构造了一个匿名对象Logger，在这个对象构造的时候其实已经写入了文件名和行号.匿名对象调用.stream()函数拿到一个LogStream对象，由这个LogStream对象重载<<将“AAA”写入LogStream的数据成员FixBuffer对象的data_缓冲区内.匿名对象在这条语句执行完毕以后会被销毁，因此会调用~muduo::Logger()函数将日志消息输出至目的地(fflush)(标准输出或者磁盘的日志文件)
   ```
6. 日志流程(非异步):`Logger`->`Implment`->`LogStream`->`operator << (即stream_ <<  写入的是GeneralTemplate对象,其实就是调用的是logstream的Append方法)`->`LogStream的FixBuffer内`->`g_output`(这是`logging.cpp`的`Logger::OutputFunc g_output`)->`g_flush`(这是`logging.cpp`的`Logger::FlushFunc g_flush`)
# asynclogging
1. 一个日志库大体分为前端和后端.前端是供应用程序使用的接口,并生成日志消息(前端是业务线程产生一条条的日志消息);后端则负责把日志消息写到目的地(后端是一个日志线程,将日志消息写入文件,日志线程只有一个)
2. <mark>前面的`logstream logfile logging`还是停留在单线程的日志考虑中,即还没涉及到前端和后端线程,`asynclogging`将它们整合形成了多线程异步日志库</mark>
3. 日志线程和业务线程之间是典型地多生产者单消费者模型,前端的多个业务线程不用考虑后端,会一直无阻塞的向后端的日志线程写日志信息
4. 由于磁盘IO是移动磁头的方式来记录文件的,其速度与CPU允许速度并不在一个数量级上,磁盘IO很慢很慢,因此在正常的实时业务处理流程中应该彻底避免磁盘IO(即业务线程中不应该有磁盘IO的出现),所以我们使用业务现场和日志线程分开
5. <mark>我们采用的是双缓冲技术(实际是四个缓冲区,这里将抽象为两个缓冲区表述):准备`buffer`A和B,前端负责往`buffer`A填日志消息(各个业务线程互斥的填),后端负责将`buffer`B的数据写入本地文件(磁盘IO).当`buffer`A写满,交换A和B,让后端将`buffer`A的数据写入文件,而前端则往`buffer`B填入新的日志消息,如此往复.用两个`buffer`可以不必等待磁盘文件的读写(如果直接在业务线程往磁盘读写,那么很可能会阻塞很久,因为磁盘IO很慢),也避免了每条新日志消息都去唤醒日志线程,我们是拼成一个大的`buffer`再唤醒后端线程的.另外,为了及时将日志消息写入文件,即便`buffer`A未满,日志库也会3秒执行依次上述交换写入操作</mark>
6. 如果前端写入速度太快,一下把前端线程(业务线程)的两块缓存都用完了,那么只好再分配一块新的`buffer`,因为此时磁盘IO还没完成,后端的`buffer`还换不到前端来,这是极少发生的情况
7. 业务线程的缓冲区队列和日志线程的缓冲区队列的交换有两种条件,`if (buffers_.empty())`:为空就说明没写满的;不为空说明有写满的
   * 超时(3秒)
   * 业务线程写满了一个或多个`buffer`
当条件满足时,先将当前缓冲`current_`移入`buffers_`,并离开将空闲的`newBuffer_current`移为当前缓冲(`current_ = std::move(newBuffer_current);`).接下来将`buffers_`与`buffersToWrite`交换(`buffersToWrite.swap(buffers_);`).最后用`newBuffer_next`代替`next_`(如果`next_`为空,说明当前没有备用缓冲区可以使用,需要从预留的`newBuffer_next`中获取一个新的缓冲区来充当`next_`),这样前端始终有一个预备的`buffer`可供调配.最终`buffersToWrite`内的`buffer`重新填充`newBuffer_current newBuffer_next`,这样下一次执行的时候还有两个空闲的`buffer`可用于替换前端的当前缓冲和备用缓冲
8. 四个缓冲区其实都是在`asynclogging`中创建的,只是前端缓冲区`current_  next_`是在业务线程中被写入的(即`void AsyncLogging::Append`在前端线程中被调用),而缓冲区交换、日志消息向文件的写入等过程都是在后端实现的.`logfile asynclogging`通常被归在后端日志线程中(但并不是说`asynclogging`的所有成员函数只会在日志线程中被调用,如前端业务线程会调用其中的`Append`,来向缓冲区写入日志信息),即负责收集日志信息和写入日志信息;`logstream logging`通常被归在前端的业务线程中,主要用于准备业务线程中的日志消息,并将其传递给`asynclogging`
9.  `logstream logfile logging asynclogging`四个文件的工作协同关系:业务线程调用`logging`的宏(如`LOG_INFO`)->构造一个`Logger::Implment`,获得一个`logstream`流->向`logstream`流写入一个`GeneralTemplate`对象->调用`asynclogging`的`Append`方法(其实就是`logstream`的`Append`方法)写入到前端线程的缓冲区->调用后端日志线程的`ThreadFunc`,`AsyncFlush`写入本地文件
10. <mark>在`asynclogging`中,不管是前端缓冲区的`current_ next_`,还是后端缓冲区的`newBuffer_current newBuffer_next`,它们都没有拷贝,而是简单的指针交换`std::move`实现,不会带来多大开销</mark>
11. 对于多线程异步日志程序,只有在日志信息大于给定的日志文件大小(`log->writebytes() >= kSingleFileMaxSize`)才会新建一个`LogFile`,即新建一个本地的`.log`文件(其它情况(3秒到了,它只是`flush`到本地,只要大小没超`kSingleFileMaxSize`,那么第二个3秒写的还是同一个本地文件(`.log`)))
12. `buffers_.reserve(16)`:对容器`std::vector<BufferPtr> buffer_`的内存预分配操作,避免频繁的内存重新分配.<mark>`std::vector`是动态数组,其大小可以根据需要增长.然而,每次增加空间时,`std::vector`需要重新分配内存,并将旧数据复制到新位置.通过调用`reserve(16)`,我们可以提前为16个元素预分配空间,减少后续添加元素时的重新分配开销</mark>
14. 《`Linux`多线程服务端编程》的P114-119:
    * 第一种情况是前端日志的频度不高,后端3秒超时后将"当前缓冲`current_`"写入文件:
    ![](asynclogging_1.png)
    * 第二种情况,在3秒超时之前已经写满了当前缓冲,于是唤醒后端线程开始写入文件:
    ![](asynclogging_2.png)  
    * 第三种情况,前端在短时间内密集写入日志消息,用完了两个缓冲,并重新分配了一块新的缓冲:
    ![](asynclogging_3.png) 
    * 第四种情况,文件写入速度较慢,导致前端耗尽了两个缓冲,并分配了新缓冲:
    ![](asynclogging_4.png) 
# 所遇问题
1. 由于路径不正确的`segmentation fault`:写入日志的本地文件路径错误
2. 本地文件出现乱码:`stream_ << GeneralTemplate(data, len)`这里传入的长度`len`如果大了,那么写入的日志文件就会乱码.因为在`logstream`的`append()`中,指针`cur_`会多向前移动一个位置,但是这个位置啥也没有,最终导致写入乱码;`len`小了,不会乱码,只是导致日志信息缺失
3. `std::strlen`与`sizeof()`的区别:
   * `std::strlen`:计算`C`风格字符串(以`\0`结尾的字符数组)的长度,计算字符数组的字符数(不包括`\0`).它只适用与以`\0`结尾的字符数组
   * `sizeof()`:这是一个编译时的运算符,用于计算数据类型或对象在内存中的大小,以字节为单位
4.  `asynclogging`的调用出现死锁:经过几个小时的验证,最终发现死锁不是由于主线程(`asynclogging`中创建日志线程的原线程)和新创建的子线程(日志线程)之间调用(`CutDown() wait()`)导致`Latch`中锁的死锁,因为它们之间没有出现同一个线程持有同一把锁的情况,`CutDown()`是在线程1中执行,然后对有着同一个`latch_`对象的线程2产生唤醒操作.真正导致死锁的原因是:`condition.h`定义的条件变量对象,之前使用的是`std::unique_lock<std::mutex> lock(mutex_.mutex());`:此时在`asynclogging`的`ThreadFunc`一开始就上锁了,如果这还上锁那么就会死锁.解决办法:`std::unique_lock<std::mutex> lock(mutex_.mutex(), std::defer_lock);`:`std::defer_lock`表示不调用`std::unique_lock  std::lock_guard`时就立即上锁,而是后面需要上锁自己手动`lock()`
5. 忘写`written_bytes_ += len;`,导致一直不能新建`LogFile`,即就算输出的日志超过了给定的日志文件最大限制`kSingleFileMaxSize`,它也不能新建一个本地文件进行写入
6. `LogFile`的`Flush`和`append`函数在使用互斥锁前,一定要先判断是否存在,即`if(mutex_)`.必须先判断,不然程序有bug! 这个`mutex_`可能在其它地方被销毁了,而此时还没有`new`一个`LogFile`(一个`LogFile`有一个属于自己的局部变量`std::unique_ptr<MutexLock> mutex_;`),那么此时就会出现卡死的bug(因为`mutex_`已经被销毁了)
7. 如果写入的日志消息很小,占不满一个缓冲区,并且业务线程在3秒内就`Stop`了日志线程,那么本地文件啥也写不进来
8. 如果某个业务线程只做一个动作:往日志线程中写日志,它一直源源不断写日志.这个操作在实际本地日志文件中可能会出现"丢包"的情况,即有些日志在传输中被丢失了,因此往往不可能一个业务线程只做往日志线程写日志的动作,只要有了一定的其它动作处理,就能保证业务线程的日志传入日志线程不会太快,此时就能解决"丢包"的问题(业务线程写日志的速度过快,以至于缓冲区无法及时被清空,可能会触发缓冲区溢出,导致部分日志数据丢失)
9. 当前端拼命发送日志消息,超过后端的输出到本地的能力,会导致日志消息在内存中的堆积,实际上就是生产者速度高于消费者速度导致的.为了解决这个问题,我们和`muduo`一样的思路:直接丢掉多余的日志`buffer`(只留两个`buffer`),腾出内存(`asynclogging.cpp`的82-94行)
# Latch
1. 在多线程环境中,启动一个新线程后,主线程或其他管理线程可能需要确保新线程已经完成初始化和准备好处理任务.`latch_.wait();`的作用就是在启动线程后等待线程内部的初始化操作完成(即当新创建的线程初始化完毕并且准备好处理任务后,那么就会`latch_.CountDown()`),然后再继续执行主线程或其他线程的操作
2. 在多线程应用中,有时主线程或其他线程需要确保新创建的线程已经完成了必要的初始化和准备工作,才能继续后续的操作.`Latch`可以帮助实现这一点,通过在启动线程前等待`Latch`的计数器减为0,然后再继续执行
3. 使用`Latch`可以控制多个线程的启动顺序和执行顺序.如:一个线程可能依赖于另一个线程的某些操作完成后才能继续执行,通过 `Latch`可以实现等待另一个线程的完成(即在另一个线程完成时才会给`latch_.CountDown()`),这就有点像修某门课程时需要先修某些课程
4. 如:线程A需要确保新创建的线程B C D已经完成了必要的初始化和准备工作,那么在线程A中初始`latch_ = 3`,然后`wait`;随后每当一个线程B C D准备好了,就会在它们自己线程中`CutDown`(这里的它们肯定是共用一个`latch_`)
# thread
1. 我们给每个线程都取了唯一的名称,为了方便输出和调试`::prctl(PR_SET_NAME, thread_name_.size() == 0 ? ("WorkThread " + string(buf)).data() : thread_name_.data());`
## 所遇问题
1. 为什么要用`latch`计数器?
   * 原因见上面的`Latch`的作用
2. 析构时为什么不是`.join()`,而是`.detach`?
   * 为了避免阻塞等待每个线程的结束,而是通过标记线程为分离状态,让每个消除在完成后(调用`thread`析构函数)自动管理其资源,这种方式可以提高程序的响应率和效率.若要使用`.join()`,可以直接调用`thread.cpp`的`Join()`
3. 为什么要先判断`.joinable()`?
   * 判断此线程是否可`detach()`或`join()`
# Cmake的学习
1. 直接利用`CMakeLists.txt`对当前目录下的某个`.cpp`文件(在当前目录下)生成可执行文件:
   ```txt
   cmake_minimum_required(VERSION 3.0)# 此项目要求的最低 CMake 版本为 3.0

   project(Timestamp_test CXX)# 定义了项目的名称为 WebServer，并且项目的主要编程语言是 C++

   include_directories(# 指定了编译器在查找头文件时应搜索的目录
   ../Base # 上一级文件夹下的Base目录  因为Base文件夹在CMakeLists.txt的上一级目录下,所以表示称../Base
   )

   aux_source_directory(Tests SRC_LIST1)# 查找当前目录下的所有源文件，并将名称保存到变量 SRC_LIST1
   add_executable(Timestamp_test Timestamp_test.cpp ${SRC_LIST1})# 指定生成目标
   ```
2. 利用`CMakeLists.txt`生成当前目录的静态库:
   ```txt
   cmake_minimum_required(VERSION 3.0)
   # 设置项目名称
   project(NetLib)
   # 设置默认的构建类型为 Release，如果用户未指定
   # Release:在 Release 模式下，编译器会对代码进行各种优化，以提高运行时性能。这些优化包括函数内联、循环展开、删除未使用的代码等
   # 在 Debug 模式下，编译器会生成完整的调试信息，使得可以在调试器中查看源代码、变量值、堆栈信息等
   if(NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE "Release")
   endif()
   # 添加当前目录到搜索路径中
   include_directories(${CMAKE_CURRENT_SOURCE_DIR})
   # 定义源文件列表
   # 这里列出了所有要包含到静态库中的 .cpp 文件
   set(NET_SOURCES
      ./Poller/DefaultPoller.cpp
      ./Poller/Epoller.cpp
      ./Poller/Poller.cpp
      ./Timer/Timer.cpp
      ./Timer/TimerQueue.cpp
      ./Util/Channel.cpp
      ./Util/CurrentThread.cpp
      ./Util/EventLoop.cpp
   )
   # 添加静态库目标 webserver_poller
   add_library(webserver_net ${NET_SOURCES})
   target_link_libraries(webserver_net webserver_base)#链接到webserver_base库
   # 标准库不需要链接
   # 安装目标文件（静态库）
   # 将构建的静态库安装到 /usr/local/include/lib 目录
   install(TARGETS webserver_net DESTINATION lib)
   # 安装头文件
   # 获取当前目录下所有以 .h 结尾的头文件并安装到 /usr/local/include/webserver 目录
   file(GLOB HEADERS "*.h")
   install(FILES ${HEADERS} DESTINATION include/webserver/Net)
   ```
   构建了`webserver_poller`静态库,在其它`CMakeLists.txt`中要链接这个库的时候,直接`target_link_libraries(webserver_util webserver_poller)`:将指定目标`webserver_util`链接到库`webserver_poller`,需要注意的是,这个指定目标可以是可执行文件、静态库或动态库,即这个函数作用是可执行文件、静态库或动态库与其它库进行链接
3. `cmake ..`:这一步会根据`CMakeLists.txt`文件生成构建文件
4. `make`:这一步会实际编译源代码(如果`CMakeLists.txt`中有构建静态库,则这一步也会构建静态库)
5. `make install`:如果`CMakeLists.txt`中有安装静态库和头文件的步骤,则这一步会将生成的静态库和头文件安装到指定的目标目录(`/usr/local/lib`和`/usr/local/include/webserver`目录
6. 通过链接静态库进行编译:
   ```txt
   cmake_minimum_required(VERSION 3.0)# 此项目要求的最低 CMake 版本为 3.0
   project(Test CXX)
   add_executable(Timestamp_test Timestamp_test.cpp)# 指定生成目标
   target_link_libraries(Timestamp_test webserver_base)
   ```
   `target_link_libraries`不需要显示指定库的位置,因为`CMake`会自动查找并链接目标所需的库,它默认会搜索几个标准的库搜索路径,如:`/usr/lib    /usr/local/lib`等
7. <mark>为了合理实现可以相互调用静态库的方法写`CMakeLists.txt`,(前提假设:下层不能调用上层,上层可以调用下层,相同层之间可能会相互调用)我们必须相同层次的目录不能单独生成静态库,即想象成网络通信时,就是相同层不要单独构建一个库,需要把相同层的源文件构建在同一个库中.如果把相同层构建为两个不同的库,这可能是完成不了的,比如:`Timer`目录文件会调用`Util`中的文件,而`Util`文件也会调用`Timer`文件,所以此时就不知道怎么构建库了,因此对于相同层我们应该构建在一个库中.本项目,我们假定了三层,一层为底层`webserver_base`(包括`./Base/Logging`和`Base`)库,第二层为网络层`webserver_net`库(包括`Poller Timer Util`),第三层为应用层`webserver_http`(`Http`)</mark>
# Shell脚本
1. 可以将`cmake`的构建过程用`Shell`脚本给出,如:
   ```bash
   # #!/bin/sh 制定了脚本解释器为/bin/sh,即使用Shell解释器来执行脚本内容  /bin/bash  就是用Bash解释器来执行脚本内容
   #!/bin/sh   

   # 这行命令启用了脚本的调试模式,它会使Shell在执行每一条命令之前,先打印出该命令及其参数,这样可以在执行过程中看到具体执行的命令,有助于调试
   set -x      

   if [ ! -d "./build" ]; then  # 检查当前目录下是否存在 ./build,不存在就创建  build用于存放cmake产生的中间文件
   mkdir ./build 
   fi
   if [ ! -d "./logfiles" ]; then  # 检查当前目录下是否存在 ./logfiles,不存在就创建  logfiles存储的是输出的日志  logfile.cpp中给定了默认的日志输出路径,就是logfiles文件夹
   mkdir ./logfiles 
   fi
   cd ./build
   cmake .. 
   make -B   # 强制重新编译所有目标文件   强制 make 忽略时间戳和依赖关系,不管源文件是否改变,总是重新构建所有目标文件
   make install
   ```
2. <span style="color:red;"> 在`Windows`上编写完脚本,拷贝到`linux`上执行时,发现会报错:
   ```
   $'\r': command not found
   invalid option: set: usage: set [-abefhkmnptuvxBCHP] [-o option-name] [--] [arg ...]
   build.sh: line 10: syntax error: unexpected end of file
   ```
   其实代码看上去一点错没有,但是会报错.这些问题通常与脚本的文本格式有关,特别是换行符的问题,可能是由于不同操作系统之间的转换造成的,为了解决这个脚本的换行符的格式,即将文件中的`\r\n`转换为`\n`,从而消除换行符问题:
   ```bash
   sed -i 's/\r$//' build.sh
   ```
3. <mark>在本项目中,我们对`Net`目录和`Base`目录都写了自动化`build.sh`脚本,而`build.sh`脚本相当于对`CMakeLists`执行`cmake`命令,即:`cmake ..  make  make install`.除了测试程序(`tests`目录),其它文件夹(`Base  Net`)执行脚本后会在`/usr/local/lib`中生成对应的静态库(不会生成可执行文件),然后在测试文件夹下的`CMakeLists.txt`直接调用静态库就行,而不用一个一个的包括源文件.测试程序(`tests`目录)中的`CMakeLists.txt`执行后是直接在目录中生成可执行文件</mark>
4. 本项目中测试程序中直接调用静态库,如`Timestamp_test.cpp`测试程序:
   ```txt
   cmake_minimum_required(VERSION 3.0)# 此项目要求的最低 CMake 版本为 3.0

   project(Test CXX)

   add_executable(Timestamp_test Timestamp_test.cpp)# 指定生成目标
   target_link_libraries(Timestamp_test webserver_base)
   ```
5. `build.sh`脚本文件<=>手动在`CMakeLists.txt`的文件夹下执行以下命令:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   make install
   ```
6. `build.sh`的执行:`sudo bash build.sh`
# RAII
1. `RAII`是`C++`中一种重要的资源管理方法.它利用对象的生命周期管理资源(如内存、文件句柄、网络连接等),确保资源在使用期间被安全地获取和释放.这种模式在防止资源泄露和确保异常安全性方面起着关键作用
2. <mark>`RAII`的核心思想是将资源的获取和释放绑定到对象的生命周期:</mark>
   * 资源获取:在对象的构造函数中获取资源
   * 资源释放:在对象的析构函数中释放资源.如:
   ```C++
   1.
   class MutexLockGuard : public NonCopyAble {
        public:
            explicit MutexLockGuard(MutexLock& mutex)
                     : mutex_(mutex) {
                       mutex_.lock();//获取锁的所有权资源
            }
            ~MutexLockGuard() {
                mutex_.unlock();//释放锁的所有权
            }
        private:
            MutexLock& mutex_;
    };
    2.
    Epoller::Epoller(EventLoop* loop)
      : Poller(loop),
         epollfd_(::epoll_create1(EPOLL_CLOEXEC)),//获取套接字资源
         events_(kDefaultEvents) {}
    Epoller::~Epoller() {
      ::close(epollfd_);//释放套接字资源
    }
    ```
3. 智能指针的内存管理实际上就是`RAII`的应用,它只是封装成API了,在调用它时就是构造函数获取内存资源(这个指针资源),在离开作用域时就会自动释放这个资源;`std::lock_guard<std::mutex> lock(mtx);`这类互斥锁也是`RAII`的应用,原理和智能指针一样,封装成了上层API,本项目相当于是自己写了一个`std::lock_guard<std::mutex>`
4. `RAII`的优点:
   * 异常安全性:通过在析构函数中释放资源,`RAII`确保了即使在异常情况下,资源也能被正确释放,防止资源泄露
   * 简化代码:`RAII`将资源管理的责任交给对象,减少了手动管理资源的需求,使代码更简洁和易于维护
   * 资源的自动管理:资源的获取和释放与对象的生命周期绑定,确保资源在需要时被分配,在不需要时被释放
5. <mark>本项目和`muduo`一样广泛使用`RAII`这一方法来管理资源的生命周期</mark>
6. 本项目所有的资源管理都是自己重新写的`RAII`对象,没有直接调用`C++`标准库中现成封装好的`RAII`对象,如:没有直接用`std::lock_guard<std::mutex>`,而是重新建了一个`MutexLock`对象;









