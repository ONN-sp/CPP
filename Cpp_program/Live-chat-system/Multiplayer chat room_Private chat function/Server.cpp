#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <unistd.h>
#include <unordered_map>
#include <arpa/inet.h>//linux提供的头文件
#include <sys/socket.h>//linux提供的头文件

#define PORT 8080
#define BUFFER_SIZE 2048
#define MAX_CLIENTS 10

std::vector<int> clients;//存储所有客户端的套接字描述符
std::mutex mtx;//互斥锁,用于保护共享资源 clients
std::mutex mtx2;//互斥锁,用于保护共享资源 client_names
std::unordered_map<int, std::string> client_names;//存储客户端名称与ID的键值对

//处理与客户端的通信
void handleClient(int cfd){
    int client_socket = cfd;
    char buffer[BUFFER_SIZE];
    while(true){
	    memset(buffer, 0, BUFFER_SIZE);
   	    int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
	    std::cout << buffer << std::endl;
            if(bytes_read <= 0){
            	std::cerr << "Client " << " disconnected" << std::endl;
            	//加锁，共享资源clients
            	mtx.lock();
                clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
                mtx.unlock();
                close(client_socket);
                return;            
        	}
        std::string message = buffer;
	    message = message.substr(message.find(':')+1);
        //判断是否为私聊消息
        if(message.find('@')==0){//若要私聊,第一个字符就是@
            //解析私聊消息
            int pos=message.find(' ');
            std::string recipient = message.substr(1, pos-1);//复制私聊对象的客户端名称,放进recipient中
            message = message.substr(pos+1);//保存发送的内容
            //查找收件人的socket id
            int recipient_socket = -1;
	        mtx2.lock();
            for(auto &pair:client_names){
                if(pair.second==recipient){
                    recipient_socket=pair.first;
                    break;
                }
            }
	        mtx2.unlock();
            //发送私聊消息给收件人
            if (recipient_socket != -1) {
                std::string sender_name = client_names[client_socket];
                std::string private_message = "[Private][" + sender_name + "]: " + message;
                send(recipient_socket, private_message.c_str(), private_message.size(), 0);
            } else {
                std::string error_message = "Error: User not found!";
                send(client_socket, error_message.c_str(), error_message.size(), 0);
            }
        }
        else{
       	  //加锁，共享资源clients
	    mtx.lock();
       	  //将收到的消息发送给所有其它客户端
          for(int client:clients){
              if(client!=client_socket){
                send(client, buffer, bytes_read, 0);
              }
          }
	    mtx.unlock();
        }
    }
}

int main() {
    int server_fd, cfd;//监听套接字，服务端的socket接口
    int opt = 1;
    sockaddr_in address;//使用sockaddr_in结构体的原因是使sockaddr结构体中IP和port的赋值变得简单，否则就需要使用指针来移动ip和port到sockaddr.sa_data数组中去
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

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
    std::cout << "这是一个多客户端和单服务器的在线聊天系统(无私聊功能)" << std::endl;
    std::cout << "客户端向服务器发送消息,然后服务器将此客户端的消息转送给其它客户端" << std::endl;

    // 接受传入连接并为每个连接启动一个线程
    while(true){
        if ((cfd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {//cfd表示通信时的套接字标识符
        std::cerr << "accept" << std::endl;
        return -1;
    }
    // 接收客户端的用户名
    char username[BUFFER_SIZE];
    memset(username, 0, BUFFER_SIZE);
    recv(cfd, username, BUFFER_SIZE, 0);
    std::cout << "用户" << username << "加入进聊天系统了!"  << std::endl;//首先输出用户信息
    std::string client_name=username;

    // 加锁，操作共享资源clients和client_names
    mtx.lock();
    clients.emplace_back(cfd);
    mtx.unlock();
    mtx2.lock();
    client_names[cfd] = client_name;
    mtx2.unlock();
    //创建一个新线程来处理与客户端的通信
    std::thread thread1(handleClient, cfd);
    thread1.detach();
    }
    close(server_fd);
    return 0;
}



