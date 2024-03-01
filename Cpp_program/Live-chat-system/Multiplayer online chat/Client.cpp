#include <iostream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>//linux提供的头文件
#include <sys/socket.h>//linux提供的头文件

#define PORT 8080//服务器端口
#define BUFFER_SIZE 1024

void sendThread(int sock, char username[]){
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        send(sock, "Client ", 7, 0);
        send(sock, username, strlen(username), 0);
        send(sock, " say:", 5, 0);    
        send(sock, message.c_str(), message.size(), 0);
    }
}

void recvThread(int sockfd) {
    char buffer[1024];
    while (true) {
        int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Error receiving data or connection closed" << std::endl;
            close(sockfd);
            return;
        }
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    int sock = 0;
    sockaddr_in serv_addr;
    
    char username[BUFFER_SIZE] = {0};
    // 创建监听的套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;//地址族协议
    serv_addr.sin_port = htons(PORT);//主机序列->网络序列,小端->大端

    // 指定客户端实际ip地址
    if(inet_pton(AF_INET, "192.168.118.128", &serv_addr.sin_addr) <= 0) {//十进制ip->二进制ip,"192.168.118.128"是客户端IP
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }
    std::cout << "Client IP is " << "192.168.118.128" << std::endl;
    // 连接服务器IP和端口
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){//客户端不需要使用函数显示的绑定PORT，因为在connect之后就会随机的给客户端分配一个没有被占用的端口
        std::cerr << "Connection error" << std::endl;
        return -1;
    }
    std::cout << "光标位于空行的首位就是处于待输入状态" << std::endl;
    memset(username, 0, BUFFER_SIZE);
    std::cout << "Client username: ";
    std::cin.getline(username, BUFFER_SIZE);
    send(sock, username, strlen(username), 0);
    // 连接成功,进行通信  
    std::thread sendThr(sendThread, sock, username);//发送线程
    std::thread recvThr(recvThread, sock);//接收线程  
    sendThr.join();
    recvThr.join(); 
    close(sock);
    return 0;
}




