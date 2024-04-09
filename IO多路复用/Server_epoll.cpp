#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>//linux提供的头文件
#include <sys/socket.h>//linux提供的头文件
#include <sys/epoll.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, cfd;//监听套接字，服务端的socket接口
    sockaddr_in address;//使用sockaddr_in结构体的原因是使sockaddr结构体中IP和port的赋值变得简单，否则就需要使用指针来移动ip和port到sockaddr.sa_data数组中去
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
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
    int epfd = epoll_create(1);// 1.
    if(epfd < 0){
        std::cerr << "epoll create" << std::endl;
        exit(0);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;// !!!
    int ep_fd = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);// 2.
    if(ep_fd < 0){
        std::cerr << "epoll ctl" << std::endl;
        exit(0);
    }
    
    struct epoll_event evs[1024];
    int size = sizeof(evs)/sizeof(evs[0]);
    while(1){
        int num = epoll_wait(epfd, evs, size, -1);// 3.
        for(int i=0; i<num; i++){
            int fd = evs[i].data.fd;// !!!
            if(fd == server_fd){//监听文件描述符
                if ((cfd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {//cfd表示通信时的套接字标识符
                    std::cerr << "accept" << std::endl;
                    return -1;
                }
                ev.events = EPOLLIN;
                ev.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
            }
            else{
                char buffer[1024];
                memset(buffer, 0, BUFFER_SIZE);
                int valread = recv(fd, buffer, BUFFER_SIZE, 0);
                if (valread < 0) {
                    std::cerr << "Recv error" << std::endl;
                    exit(1);
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
        }
    }
    close(server_fd);
    return 0;
}
