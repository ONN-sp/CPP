//主线程:调用epoll_wait()
//子线程1(实际上上N个):用于accept
//子线程2(实际上是N个):用于通信(recv,send) 
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>//linux提供的头文件
#include <sys/socket.h>//linux提供的头文件
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <mutex>
#include <thread> 

#define PORT 8080
//待传输到子线程的参数用结构体表示
typedef struct socketinfo
{
    int fd;
    int epfd;
}SocketInfo;

//子线程1
void acceptConn(SocketInfo* info, sockaddr_in address, int addrlen){
    int cfd;
    std::thread::id threadId = std::this_thread::get_id();
    std::cout << "子线程ID: " << threadId << std::endl;
    if ((cfd = accept(info->fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {//cfd表示通信时的套接字标识符
        std::cerr << "accept" << std::endl;
        delete info;
        return;
    }
    //将fd套接字属性设置为非阻塞
    struct epoll_event ev;
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = cfd;
    epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
    delete info;
    return;
}

//子线程2
void communication(SocketInfo* info){
    char buffer[5];
    int fd = info->fd;
    int epfd = info->epfd;
    while(1){// 这个循环是为了读完一次的数据
        memset(buffer, 0, sizeof(buffer));//先清空再读取
        int valread = recv(fd, buffer, sizeof(buffer), 0);
        if (valread == -1) {
            if(errno == EAGAIN){// 为了防止非阻塞时缓冲区为空的报错
                std::cout << "读取完毕" << std::endl;
                break;
            }
            else{
                std::cerr << "Recv error" << std::endl;
                break;
            }
        }
        else if(valread == 0){
            std::cerr << "Connection closed by client" << std::endl;
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            break;
        }
        std::cout << "Client say: " << buffer << std::endl;
        // 发送给客户端                         
        std::cout << "Server say: ";
        send(fd, buffer, strlen(buffer), 0);
        std::cout << buffer << std::endl;
    }
    delete info;
    return;
}

int main() {
    int server_fd, cfd;//监听套接字，服务端的socket接口
    sockaddr_in address;//使用sockaddr_in结构体的原因是使sockaddr结构体中IP和port的赋值变得简单，否则就需要使用指针来移动ip和port到sockaddr.sa_data数组中去
    int addrlen = sizeof(address);
    int opt = 1;

    // 创建监听的套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){//AF_INET=ipv4，SOCK_STREAM:流式传输协议(如TCP)，SOCK_STREAM+0表示使用流式传输协议中的TCP
        std::cerr << "socket error" << std::endl;
        return -1;
    }
   // 设置套接字选项以允许地址重用
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "setsockopt" << std::endl;
        exit(EXIT_FAILURE);
    }
    // 绑定本地的IP和端口
    address.sin_family = AF_INET;//地址族协议,IPV4
    address.sin_addr.s_addr = INADDR_ANY;//初始化IP，不是绑定的0.0.0.0，服务器端会自动读取实际IP
    address.sin_port = htons(PORT);//主机序列->网络序列,小端->大端


    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {//sockaddr结构体是IPv4协议族对应的结构体
        std::cerr << "bind error" << std::endl;
        return -1;
    }

    // 设置监听
    if (listen(server_fd, 128) < 0) {//一次性同时监听的最大数是128,此参数默认也是128
        std::cerr << "listen" << std::endl;
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // epoll函数  IO多路复用
    int epfd = epoll_create(1);
    if(epfd < 0){
        std::cerr << "epoll create" << std::endl;
        exit(0);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    int ep_fd = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
    if(ep_fd < 0){
        std::cerr << "epoll ctl" << std::endl;
        exit(0);
    }
    
    struct epoll_event evs[1024];
    int size = sizeof(evs)/sizeof(evs[0]);
    while(1){
        int num = epoll_wait(epfd, evs, size, -1);
        for(int i=0; i<num; i++){//这个i不是指的套接字
            int fd = evs[i].data.fd;       
            if(fd == server_fd){//监听文件描述符
                SocketInfo* info = new SocketInfo;
                info->fd = fd;
                info->epfd = epfd;
                std::thread t1(acceptConn, info, address, addrlen);
                t1.detach();
            }
            else{
                    SocketInfo* info = new SocketInfo;
                    info->fd = fd;
                    info->epfd = epfd;
                    std::thread t2(communication, info);
                    t2.detach();
                }
            }
        }
    close(server_fd);
    return 0;
}
