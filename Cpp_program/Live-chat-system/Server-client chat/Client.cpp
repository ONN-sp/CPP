//second start
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>//linux提供的头文件
#include <sys/socket.h>//linux提供的头文件

#define PORT 8080//服务器端口
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 创建监听的套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;//地址族协议
    serv_addr.sin_port = htons(PORT);//主机序列->网络序列,小端->大端

    // 指定服务器实际ip地址
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {//十进制ip->二进制ip
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }
    // 连接服务器IP和端口
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){//客户端不需要使用函数显示的绑定IP和PORT，因为在connect之后就会随机的给客户端分配一个没有被占用的端口和本地IP地址
        std::cerr << "Connection error" << std::endl;
        return -1;
    }

    // 连接成功,进行通信
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);//memset函数是内存赋值函数(常用于初始化)，用来给某一块内存空间进行赋值的.第一个参数要是指针(地址)
        std::cout << "Client say: ";
        std::cin.getline(buffer, BUFFER_SIZE); 
        send(sock, buffer, strlen(buffer), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int valread = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valread <= 0) {
            std::cerr << "Server disconnected" << std::endl;
            break;
        }
        std::cout << "Server say: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}
