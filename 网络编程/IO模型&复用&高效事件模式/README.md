1. Server_select.cpp:利用`select()`函数,并且是单线程顺序处理文件描述符
2. Server_select_mt:`select()`+`多线程`.单线程的IO多路复用虽然可以实现并发,但是它的文件描述符处理还是顺序的,效率相对低(当多个客户端同时通信服务器时).因此,可以使用多线程:主线程:调用select;子线程1(实际上上N个):用于accept;子线程2(实际上是N个):用于通信(recv,send)
3. Server_epoll.cpp:利用`epoll()`相关操作函数,并且是单线程
4. Server_epoll_ET.cpp:`epoll()`+`边沿触发模式`
5. Server_epoll_ET_mt.cpp:`epoll()`+`边沿触发模式`+`多线程`
6. IO多路复用.md:IO多路复用的学习日志
7. Reactor.md:反应堆的学习日志
8. IO模型.md:IO模型学习日志