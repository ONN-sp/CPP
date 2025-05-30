1. `select()`、`poll()`、`epoll()`的对比:
   * `select`缺点：select() 检测数量有限制，最大值通常为 1024（bit）；每次调用select，都需要把fd集合从用户态拷贝到内核态，这个开销在fd很多时会很大；每次调用select都需要在内核遍历传递进来的所有fd，这个开销在fd很多时也很大
   * `poll`缺点：poll解决了select的检测文件描述符表上限的问题，但是另外两个问题还是有
   * `epoll`优点：epoll底层数据结构； 红黑树增删改综合效率高；epoll是基于事件通知而不是轮询，因此不需要遍历；epoll通过epoll_ctl添加文件描述符时是直接在内核中进行的，因此epoll不会每次调用时都从用户态到内核态进行拷贝
2. 我们编写的代码只是一个存储在硬盘的静态文件,通过编译、汇编、链接后就会生成二进制可执行文件,当我们运行这个可执行文件后,它会被装在到内存中,接着CPU会执行程序中的每一条指令,那么这个运行中的程序就是进程
3. `CPU`上下文切换就是先把前一个任务的`CPU`上下文(`CPU`寄存器和程序计数器)保存起来,然后加载新任务的上下文到这些寄存器和程序计数器,最后再跳转到程序计数器所指的新位置,运行新任务
4. 上述所说的任务主要包含进程、线程和中断.所以,`CPU`上下文切换分成:进程上下文切换、线程上下文切换和中断上下文切换
5. 进程的上下文切换不仅包含了虚拟内存、栈、全局变量等用户空间的资源,还包括了内核堆栈、寄存器等内核空间的资源
6. 线程的上下文切换:
   * 当两个线程不是属于同一个进程,则切换的过程就跟进程上下文切换一样
   * 当两个线程是属于同一个进程,因为虚拟内存是共享的,所以在切换时,虚拟内存这些资源就保持不动,只需要切换线程的私有数据、寄存器等不共享的数据
7.  一个进程切换到另一个进程运行,称为进程上下文切换
8.  进程上下文切换:会把交换的信息保存在进程的`PCB`(`PCB`是描述进程的数据结构,它是进程存在的唯一标识,它意味着一个进程的存在),当要运行另外一个进程的时候,我们需要从这个进程的`PCB`取出上下文,然后恢复到`CPU`中,这使得这个进程可以继续执行
9.   同一个进程内多个线程之间可以共享代码段、数据段、打开的文件等资源,但每个线程各自都有一套独立的寄存器和栈,这样可以确保线程的控制流是相对独立的