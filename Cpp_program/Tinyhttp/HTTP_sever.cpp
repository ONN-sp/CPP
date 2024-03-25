//服务器端
#include <iostream>
#include <sys/socket.h>
#include <fstream>
#include <sys/stat.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>

/**
 * @brief 把HTTP响应的头部信息写到套接字sock中
 * 
 * @param sock 
 * @param filename 
 */
void headers(int sock, const char* filename){
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
void not_found(int sock){
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

void cat(int sock, std::ifstream resource){}

void unimplemented(int sock){}

void execute_cgi(int sock, const char* path, const char* method, const char* query_string){}

/**
 * @brief  调用cat函数把服务器文件返回给浏览器
 * 
 * @param client 
 * @param filename 
 */
void server_file(int sock, const char* filename){
    std::ifstream resource;//声明一个输入文件流对象resource
    int num_chars = 1;
    //感觉可以不用处理buffer  ???
    char buffer[1024];//声明一个字符数组buffer
    buffer[0] = 'A';
    buffer[1] = '\0';

    while((num_chars>0)&&std::strcmp("\n",buffer))//读取并丢弃请求头信息,直到遇到空行
        num_chars = get_line(sock, buffer, sizeof(buffer));
    
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
int get_line(const int sock, char* buffer, const int size){
    int num_chars = 0;//初始化一个计数器用于记录读取的字符数
    char c = '\0';//初始化为空字符
    int n;//用于存储recv的返回值,返回的是读取的字符数
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
        else{
            c = '\n';//读取失败,则设c='\n',进而结束循环
        }
        buffer[num_chars] = '\0';//添加空字符
        return num_chars;
    }
}

/**
 * @brief 处理从套接字上监听到的一个http请求
 * 
 * @param sock 
 */
void accept_request(const int sock){
    char buffer[1024];//用于存储从客户端接收的数据
    int num_chars;//用于存储接收到的字符数
    char method[255];//用于存储HTTP请求方法
    char url[255];//用于存储请求的URL
    std::string path;//用于存储请求的文件路径
    size_t i, j;
    struct stat st;
    int cgi = 0;// 如果服务器决定这是一个CGI程序,则为真
    char *query_string = nullptr;//用于存储URL中的查询字符串

    num_chars = get_line(sock, buffer, sizeof(buffer));//从客户端接收http请求
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
        if(*query_string=='?'){//当前字符是?,则意味着查询字符串的开始
            cgi = 1;//
            *query_string = '\0';//将'?'设为'\0',目的是将url中的查询字符串和其它url分隔开
            query_string++;
        }
    }

    //构建文件路径
    path += url;//c++中可以直接将char[]拼接string
    path = path.c_str();//转换为C风格字符串,因为stat不接受string类型

    //如果路径以'/'结尾,则此时url未把index.html加进去,需要在这处理
    if(path[std::strlen(path)-1]=='/')
        std::strcat(path, "index.html");
    
    //检查请求的文件是否存在
    if(stat(path, &st)==-1){//不存在
        while((num_chars>0)&&std::strcmp("\n", buffer))//读取请求头部并丢弃,并且读取不到请求体
            num_chars = get_line(sock, buffer, sizeof(buffer));
        not_found(sock);//404
    }
    else{
        if((st.st_mode&S_IFMT)==S_IFDIR) //如果是目录
            std::strcat(path, "/index.html");//将index.html追加到路径
        if((st.st_mode&S_IXUSR)||(st.st_mode&S_IXGRP)||(st.st_mode&S_IXOTH)) //如果文件有可执行权限
            cgi = 1;
        if(!cgi)
            server_file(sock, path);//直接提供文件内容
        else    
            execute_cgi(sock, path, method, query_string);//调用CGI程序
    }
    close(sock);
}

/**
 * @brief 初始化httpd服务,建立套接字、绑定端口、进行监听
 * 
 * @param port 
 * @return int 
 */
int startup(short* port){
    int httpd_sock = 0;
    sockaddr_in address;
    //创建监听的套接字
    if((httpd_sock = socket(AF_INET, SOCK_STREAM, 0))){//AF_INET=ipv4，SOCK_STREAM:流式传输协议(如TCP)，SOCK_STREAM+0表示使用流式传输协议中的TCP
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
    address.sin_addr.s_addr = htonl(INADDR_ANY);//long类型的主机字节序->网络字节序
    address.sin_port = htons(*port);//short类型的主机序列->网络序列,小端->大端
    //绑定IP和端口
    if (bind(httpd_sock, (struct sockaddr *)&address, sizeof(address)) < 0) {//sockaddr结构体是IPv4协议族对应的结构体
        std::cerr << "bind error" << std::endl;
        return -1;
    }
    //如果端口没有设置,就提供一个随机端口
    if(*port == 0){
        socklen_t namelen = sizeof(address);//socklen_t是一种数据类型,通常用于表示套接字地址(IP+端口)结构的长度,它通常是一个无符号整数类型
        if(getsockname(httpd_sock, reinterpret_cast<sockaddr *>(&name), &namelen) == -1)
            std::cerr << "getsockname error" << std::endl;
        *port = ntohs(name.sin_port);
    }
    //监听套接字
    if (listen(httpd_sock, 128) < 0) {//一次性同时监听的最大数是128,此参数默认也是128
        std::cerr << "listen" << std::endl;
        return -1;
    }
}

int main(){
    int server_sock = -1;//监听时的套接字标识符
    short port = 80;//监听端口
    int client_sock = -1;//通信时的套接字标识符
    sockaddr_in client_name;
    int addrlen = sizeof(client_name);

    server_sock = startup(&port);//启动
    std::cout << "httpd running on port " << port << std::endl;
    while(1){
        //阻塞等待客户端的连接
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_name, (socklen_t*)&addrlen)) < 0){
            std::cerr << "accept" << std::endl;
            return -1;
        }

        std::thread new_thread(accept_request, client_sock);
        new_thread.detach();
    }
    close(server_sock);
    return 0;
}
