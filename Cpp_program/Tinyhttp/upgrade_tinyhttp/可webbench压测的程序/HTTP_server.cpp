//线程池+EPOLL+REACTOR模式
#include <iostream>
#include <sys/socket.h>
#include <fstream>
#include <sys/stat.h>
#include <cstring>
#include <string>
#include <cctype>
#include <cstdlib>
#include <thread>
#include <functional>
#include <sys/wait.h>
#include "HTTP_server.h"
#include <unistd.h>
#include <arpa/inet.h>
#include "ThreadPool.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

/**
* @brief 构建一个新的 http 服务器对象
* 
*/
HTTP_server::HTTP_server(){};

/**
 * @brief 把HTTP响应的头部信息写到套接字sock中
 * 
 * @param sock 
 * @param filename 
 */
void HTTP_server::headers(int sock, const char* filename){
    char buffer[1024];
    (void)filename;//这一行为了告诉编译器我们有意不使用这个参数,而使编译器不会发出未使用参数的警告
    std::string response;

    response += "HTTP/1.1 200 OK\r\n";
    send(sock, response.c_str(), response.length(), 0);

    response.clear();
    response += "Server: my server\r\n";
    send(sock, response.c_str(), response.length(), 0);

    response.clear();
    response += "Content-Type: text/html\r\n";
    send(sock, response.c_str(), response.length(), 0);

    response.clear();
    response += "\r\n";
    send(sock, response.c_str(), response.length(), 0);
}

/**
 * @brief 主要处理找不到请求的文件时向浏览器输出的html提示信息---404
 * 
 * @param sock 
 */
void HTTP_server::not_found(int sock){
    std::string buffer;

    buffer += "HTTP/1.1 404 NOT FOUND\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "Server: my server\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "Content-Type: text/html\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);
    
    //发送http响应报文的主体信息
    buffer.clear();
    buffer += "<HTML><TITLE>Sorry, not found!</TITLE>\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "<BODY><P>The server could not fulfill\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "Your request because the resource specified\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "is unavailable or nonexistent.\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "</BODY></HTML>\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);
}

/**
 * @brief 返回给客户端这是个错误请求 400
 * 
 * @param sock 
 */
void HTTP_server::bad_request(int sock){
    std::string buffer;

    buffer += "HTTP/1.1 400 BAD REQUEST\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "Content-Type: text/html\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "<P>Your browser sent a bad request\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);
    
    buffer.clear();
    buffer += "such as a POST without a Content-Length.\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);
}

/**
 * @brief 主要处理发生在执行CGI程序时出现的错误
 * 
 * @param sock 
 */
void HTTP_server::cannot_execute(int sock){
    std::string buffer;

    buffer += "HTTP/1.1 500 Internal Server Error\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "Content-Type: text/html\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "<P>Error prohibited CGI execution.\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);
}

/**
 * @brief 把错误信息写到cerr,并退出
 * 
 * @param sc 
 */
void HTTP_server::error_die(const char* sc){
    std::cerr << sc << std::endl;
    exit(1);
}

/**
 * @brief 读取服务器上某个文件写到socket套接字中
 * 
 * @param sock 
 * @param resource 
 */
void HTTP_server::cat(int sock, std::ifstream& resource){
    std::string line;
    // 逐行读取文件内容并发送给客户端(不是一个字符串(空白字符分割)读取:resource >> buffer)
    while(std::getline(resource, line)){
        // 发送数据给客户端
        send(sock, line.c_str(), line.length(), 0);
    }
}

/**
 * @brief 返回给浏览器表明收到的HTTP请求所用的method不被支持
 * 
 * @param sock 
 */
void HTTP_server::unimplemented(int sock){
    std::string buffer;

    buffer += "HTTP/1.1 501 Method Not Implemented\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "Server: my server\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "Content-Type: text/html\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);
    
    //发送http响应报文的主体信息
    buffer.clear();
    buffer += "<HTML><HEAD><TITLE>Method Not Implemented\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "</TITLE></HEAD>\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "<BODY><P>HTTP request method not supported.\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);

    buffer.clear();
    buffer += "</BODY></HTML>\r\n";
    send(sock, buffer.c_str(), buffer.length(), 0);
}

/**
 * @brief 运行CGI程序
 * 
 * @param sock 
 * @param path 
 * @param method 
 * @param query_string 
 */
void HTTP_server::execute_cgi(int fd, int epfd, const char* path, const char* method, const char* query_string){
    char buffer[1024];
    int cgi_output[2];//用于CGI输出的管道   cgi_output[0]为输出管道的读取端    cgi_output[1]为为输出管道的写入端
    int cgi_input[2];//用于CGI输入的管道    cgi_input[0]为输出管道的读取端    cgi_input[1]为为输出管道的写入端
    pid_t pid;//进程ID
    int status;//进程状态
    char c;
    int num_chars = 1;//读取的字符数
    int content_length = -1;//请求内容的长度
	int sock = fd;

    buffer[0] = 'A';
    buffer[1] = '\0';
    //如果是GET请求,则读取并丢弃请求头,直到读取到空行(说明后面就是请求正文了)
    if(std::strcmp(method, "GET") == 0)
        while((num_chars>0)&&std::strcmp("\n", buffer))
            num_chars = get_line(sock, epfd, buffer, sizeof(buffer));
    else{//如果是POST请求
        num_chars = get_line(sock, epfd, buffer, sizeof(buffer));
        while((num_chars>0)&&std::strcmp("\n", buffer)){
            buffer[15] = '\0';//15的原因是为了截取Content-Length头部信息
            if(std::strcmp(buffer, "Content-Length:") == 0)
                content_length = std::stoi(&(buffer[16]));
            num_chars = get_line(sock, epfd, buffer, sizeof(buffer));
        }
        //如果请求内容长度无效,返回错误
        if(content_length == -1){
            bad_request(sock);
            return;
        }
    }
    //发送HTTP响应头
    memset(buffer, '\0', sizeof(buffer));
    std::strcpy(buffer, "HTTP/1.1 200 OK\r\n");
    send(sock, buffer, std::strlen(buffer), 0);
    //创建CGI输出管道
    if(pipe(cgi_output) < 0){
        cannot_execute(sock);
        return;
    }
    //创建CGI输入管道
    if(pipe(cgi_input) < 0){
        cannot_execute(sock);
        return;
    }
    //创建子进程执行CGI脚本
    if((pid=fork()) < 0){//在fork()调用之后,父进程和子进程都会一起执行下面的代码
        cannot_execute(sock);
        return;
    }
    if(pid==0){//pid==0是用来确定当前是子进程.  子进程:执行CGI程序
        //CGI环境变量
        char meth_env[255];
        char query_env[255];
        char length_env[255];
        //重定向标准输入输出到管道
        dup2(cgi_output[1], 1);//重定向子进程的标准输出到(输出)管道的写入端(即标准输出的内容被发送到管道的写入端而不是屏幕);
        dup2(cgi_input[0], 0);//重定向子进程的标准输入到(输入)管道的读取端(即标准输入的内容不是从键盘得到,而是管道的读取端)
        close(cgi_output[0]);
        close(cgi_input[1]);
        //设置请求方法环境变量
        std::strcpy(meth_env, "REQUEST_METHOD=");
        std::strcat(meth_env, method);
        putenv(meth_env);//形参类型为char*
        if(std::strcmp(method, "GET") == 0){
            std::strcpy(query_env, "QUERY_STRING=");
            std::strcat(query_env, query_string);
            putenv(query_env);
        }
        else{//POST请求,设置内容长度环境变量
            std::strcpy(length_env, "CONTENT_LENGTH=");
            std::strcat(length_env, std::to_string(content_length).c_str());
            putenv(length_env);
        }
        execl(path, path, nullptr);//执行CGI脚本,它的输出会被写入到cgi_output管道中 
        exit(0);   
    }
    else{//父进程
        close(cgi_output[1]);//关闭输出管道的写端
        close(cgi_input[0]);//关闭输入管道的读端
        //如果是POST请求,向输入管道的写端写入数据
        if(std::strcmp(method, "POST") == 0)
            for(int i=0;i<content_length;i++){
                recv(sock, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        //从CGI输出管道的读取端读取数据并发送给客户端(execl()执行cgi程序后会把其标准输出写入到cgi_output中)
        while(read(cgi_output[0], &c, 1) > 0)
            send(sock, &c, 1, 0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        //等待子进程结束
        waitpid(pid, &status, 0);    
    }
}

/*
 * @brief  调用cat函数把服务器文件返回给浏览器
 * 
 * @param client 
 * @param filename 
 */
void HTTP_server::server_file(int fd, int epfd, const char* filename){
    std::ifstream resource;//声明一个输入文件流对象resource
    int num_chars = 1;
    //感觉可以不用处理buffer  ???
    char buffer[1024];//声明一个字符数组buffer
    buffer[0] = 'A';
    buffer[1] = '\0';
	int sock = fd;

    while((num_chars>0)&&std::strcmp("\n",buffer))//读取并丢弃请求头信息,直到遇到空行
        num_chars = get_line(fd, epfd, buffer, sizeof(buffer));
    
    resource.open(filename);//打开filename文件,并将其关联到resource对象
    if(!resource.is_open())//打开失败
        not_found(sock);
    else{
        headers(sock, filename);//调用headers,给客户端回复HTTP响应行+响应头部信息
        cat(sock, resource);//调用cat函数,发送文件内容
    }
    resource.close();//关闭文件资源
}

/**
 * @brief 只要发现读取的数据c为'\n'就认为是一行结束;如果读到'\r',就再用MSG_PEEK的方式查看下一个字符,如果是'\n',则从socket中读出;否则将回车符转换为换行符
 * 
 */
int HTTP_server::get_line(int fd, int epfd, char* buffer, const int size){
    int num_chars = 0;//初始化一个计数器用于记录读取的字符数
    char c = '\0';//初始化为空字符
    int n;//用于存储recv的返回值,返回的是读取的字符数
	int sock = fd;
    while((num_chars<size-1)&&(c!='\n')){
        n = recv(sock, &c, 1, 0);//从套接字sock中接收一个字符,存入c中,返回值为实际读取的字节数
        if(n>0){//成功读取字符
            if(c=='\r'){
                n = recv(sock, &c, 1 , MSG_PEEK);//读取到'\r'就再用MSG_PEEK查看下一个字符,只查看不接收
                //如果下一个字符是'\n',则接收该字符,否则将回车符转换为换行符
                if((n>0)&&c=='\n')
                    recv(sock, &c, 1, 0);//读取换行符
                else
                    c = '\n';//将回车符转换为换行符
            }
            buffer[num_chars] = c;//将读取的字符存储到缓冲区中
            num_chars++;
        }
		else if(n==0){//没有这几行在压测时会立马自动退出,因为发生了资源泄露,即本来客户端主动断开了连接,但是这个socket文件描述符没被释放,还占着资源,导致文件描述符很快被消耗完.因此，在高负载测试(`webbench`,不管客户端数和连接时间)下就会立即退出
            epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL);
            close(sock);
            break;
        }
        else{
            c = '\n';//读取失败,则设c='\n',进而结束循环
        }
    }
	buffer[num_chars] = '\0';//添加空字符
    return num_chars;
}

/**
 * @brief 处理从套接字上监听到的一个http请求
 * 
 * @param sock 
 */
void HTTP_server::accept_request(int fd, int epfd){
    char buffer[1024];//用于存储从客户端接收的数据
    int num_chars;//用于存储接收到的字符数
    char method[255];//用于存储HTTP请求方法
    char url[255];//用于存储请求的URL
    char path[512];//用于存储请求的文件路径
    size_t i, j;
    struct stat st;
    int cgi = 0;// 如果服务器决定这是一个CGI程序,则为真
    char *query_string = nullptr;//用于存储URL中的查询字符串
	int sock = fd;

    num_chars = get_line(fd, epfd, buffer, sizeof(buffer));//从客户端接收http请求
    i=0; j=0;
    //解析请求方法,http请求报文中第一行是请求方法
    while(!isspace(buffer[j])&&(i<sizeof(method)-1)){
        method[i] = buffer[j];
        i++;
        j++;
    }
    method[i] = '\0';

    //检查请求方法是否是GET或POST,如果不是则返回未实现的错误
    if(std::strcmp(method, "GET")&&std::strcmp(method, "POST")){
        unimplemented(sock);
        return;
    }

    //如果请求方法是POST,则设置CGI标志为真
    if(std::strcmp(method, "POST")==0)
        cgi = 1;
    
    //解析URL,在buffer中把url提取出来
    i = 0;
    while(isspace(buffer[j])&&(j<sizeof(buffer)))//空格则跳过
        j++;
    while(!isspace(buffer[j])&&(i<sizeof(url)-1)&&(j<sizeof(buffer))){
        url[i] = buffer[j];
        i++;
        j++;
    }
    url[i] = '\0';

    //如果请求方法是GET,则解析查询字符串
    if(std::strcmp(method, "GET") == 0){
        query_string = url;
        while((*query_string!='?')&&(*query_string!='\0'))
            query_string++;
        if(*query_string=='?'){//当前字符是'?',则意味着查询字符串的开始
            cgi = 1;//
            *query_string = '\0';//将'?'设为'\0',目的是将url中的查询字符串和其它url分隔开
            query_string++;
        }
    }

    //构建文件路径
	std::strcpy(path, "htdocs");
    std::strcat(path, url);//拼接

    //如果路径以'/'结尾,则此时url未把index.html加进去,需要在这处理
    if(path[std::strlen(path)-1]=='/')
        std::strcat(path, "index.html");
    
    //检查请求的文件是否存在
    if(stat(path, &st)==-1){//不存在
        while((num_chars>0)&&std::strcmp("\n", buffer))//读取请求头部并丢弃,并且读取不到请求体
            num_chars = get_line(fd, epfd, buffer, sizeof(buffer));
        not_found(sock);//404
    }
    else{
        if((st.st_mode&S_IFMT)==S_IFDIR) //如果是目录
            std::strcat(path, "/index.html");//将index.html追加到路径
        if((st.st_mode&S_IXUSR)||(st.st_mode&S_IXGRP)||(st.st_mode&S_IXOTH)) //如果文件有可执行权限
            cgi = 1;
		std::cout << "cgi: " << cgi << std::endl;
        if(!cgi)
            server_file(fd, epfd, path);//直接提供文件内容
        else    
            execute_cgi(fd, epfd, path, method, query_string);//调用CGI程序
    }
    close(sock);
}

/**
 * @brief 初始化httpd服务,建立套接字、绑定端口、进行监听
 * 
 * @param port 
 * @return int 
 */
int HTTP_server::startup(short* port){
    int httpd_sock = 0;
	int opt = 1;
    sockaddr_in address;
    //创建监听的套接字
    if((httpd_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std::cerr << "socket error" << std::endl;
        return -1;
    }
    //设置套接字选项以运行地址重用
    if (setsockopt(httpd_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "setsockopt" << std::endl;
        exit(EXIT_FAILURE);
    }
    //设置套接字地址结构
    address.sin_family = AF_INET;//地址族协议,IPV4
    address.sin_port = htons(*port);//short类型的主机序列->网络序列,小端->大端
	address.sin_addr.s_addr = INADDR_ANY;//long类型的主机字节序->网络字节序
    //绑定IP和端口
    if (bind(httpd_sock, (struct sockaddr *)&address, sizeof(address)) < 0) {//sockaddr结构体是IPv4协议族对应的结构体
        std::cerr << "bind error" << std::endl;
        return -1;
    }
    //如果端口没有设置,就提供一个随机端口
    if(*port == 0){
        socklen_t namelen = sizeof(address);//socklen_t是一种数据类型,通常用于表示套接字地址(IP+端口)结构的长度,它通常是一个无符号整数类型
        if(getsockname(httpd_sock, reinterpret_cast<sockaddr *>(&address), &namelen) == -1)
            std::cerr << "getsockname error" << std::endl;
        *port = ntohs(address.sin_port);
    }
    //监听套接字
    if (listen(httpd_sock, 128) < 0) {//一次性同时监听的最大数是128,此参数默认也是128
        std::cerr << "listen" << std::endl;
        return -1;
    }
	return httpd_sock;
}

/**
 * @brief 销毁 http 服务器对象
 * 
 */
HTTP_server::~HTTP_server(){};


int main(){
    int server_sock = -1;//监听时的套接字标识符
    short port = 9999;//监听端口
    int cfd = -1;//通信时的套接字标识符
    sockaddr_in client_name;
    int addrlen = sizeof(client_name);
    HTTP_server http;
    std::cout << "httpd running on port " << port << std::endl;
    ThreadPool pool(5);
    //线程池+epoll
    std::cout << "Create threadpool success" << std::endl;
    server_sock = http.startup(&port);//启动
    //创建epoll事件
    int epfd = epoll_create(1);
    if(epfd < 0){
        std::cerr << "epoll create" << std::endl;
        exit(0);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_sock;
    int ep_fd = epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &ev);
    if(ep_fd < 0){
        std::cerr << "epoll ctl" << std::endl;
        exit(0);
    }
    struct epoll_event evs[1024];
    int size = sizeof(evs)/sizeof(evs[0]);
    while(1){
        int num = epoll_wait(epfd, evs, size, -1);
        for(int i=0;i<num;i++){
            int fd = evs[i].data.fd;
            if(fd == server_sock){
                std::thread::id threadId = std::this_thread::get_id();
				std::cout << "子线程ID: " << threadId << std::endl;
				if ((cfd = accept(fd, (struct sockaddr *)&client_name, (socklen_t*)&addrlen)) < 0) {//cfd表示通信时的套接字标识符
					std::cerr << "accept" << std::endl;
					return -1;
				}
				//将fd套接字属性设置为非阻塞
				int flag = fcntl(cfd, F_GETFL);
				flag |= O_NONBLOCK;
				fcntl(cfd, F_SETFL, flag);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = cfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
            }
            else{
                std::thread::id threadId = std::this_thread::get_id();
                std::cout << "子线程ID: " << threadId << std::endl;
                pool.enqueue(std::bind(&HTTP_server::accept_request, &http, fd, epfd), fd, epfd);
            }
        }
    }      
    close(server_sock);
    return 0;
}