1. `Reactor`是一种事件驱动的设计模式,用于构建高效的并发应用程序.`select()  poll()  epoll()`是`linux`中用于实现`Reactor`模式的IO多路复用系统调用,即可以看作是`Reactor`模式的一种具体实现方法
2. 虽然`select()`的事件通知方式是使用轮询,但它也是基于事件驱动的
3. `reactor`模型是网络服务器端用来处理高并发IO请求的模型
4. `reactor`模型有三个角色:
   * `reactor`:负责事件监听和事件分发->(`select  epoll_wait等`)
   * `acceptor`:负责连接事件(`accept`)
   * `handler`:负责读写事件(`recv  send`)
5. `reactor`三种模型:
   * 单线程:`reactor acceptor handler`都是一个线程执行
   ![](单线程reactor.png)
   * 多线程:`reactor acceptor handler`都是一个线程执行,但是与此同时,会有一个线程池由若干`worker`线程组成,即此时的`handler`只负责读取请求`recv`和写回结果`send`,而具体的业务处理由`worker`线程完成
   ![](多线程reactor.png)
   * 主从多线程:一个主`reactor`线程:负责监听客户端事件,并在线程中让`acceptor`处理连接,即`select()+accept()`.多个子`reactor`线程负责连接后的后续事件处理,如`recv  send`,而具体的业务处理函数会让`worker`线程完成,即此时是相对于多线程把`handler`给放在子`reactor`线程处理了
   ![](主从多线程reactor.png)
6. `reactor`模式是一种事件驱动的模式,所以使用`select() poll() epoll()`进行IO多路复用的服务端也就是采用了`reactor`设计模式
7. `《Linux高性能服务器编程》`中说`reactor`在高并发处理时要设计成主线程只负责监听(`accept`)文件描述符上是否有事件发生,而不做任何其它实质性的工作.`accept recv send`以及对应的`task`都要在工作线程中完成
   ![](reactor工作流程.png)
8. 虽然`select()`在实现上可能采用了不同的机制(如轮询或底层操作系统提供的事件通知机制),但从调用者的角度来看,它确实是基于事件驱动的,因为程序会在事件发生时被唤醒并处理相应的事件