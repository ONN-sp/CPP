#include <iostream>
#include <string>
#include <getopt.h>
#include "EventLoop.h"
#include "Logging/logging.h"
#include "Server.h"

int main(int argc, char* argv[]){
    int thread_number = 4;//线程池中的线程数
    int port = 80;//服务器端口号
    std::string logPath = "./WebServer.log";//相对路径
    //利用getopt()解析命令行参数,如:./your_program_name -t 8 -p 8080 -l /var/log/webserver.log
    int opt;
    const char* str = "t:l:p";//t:需要设置线程数;l:需要设置日志路径;p:需要设置端口号
    while((opt = getopt(argc, argv, str))!=-1){
        switch (opt) {
            case 't':
                thread_number = std::stoi(optarg);//optarg:位于getopt.h的一个全局变量,用于存储当前解析到的选项的参数值  
                std::cout << "Thread number: " << thread_number << std::endl;
                break;
            case 'l':
                logPath = optarg;
                if(logPath.size()<2||optarg[0]!='/'){//如果路径名不合理,则立即终止程序
                    std::cout << "LogPath should be start with \"/\"" << std::endl;
                    exit(1);//程序异常终止
                }
                std::cout << "Log path: " << logPath << std::endl;
                break;
            case 'p':
                port = std::stoi(optarg);//std::stoi<=>atoi()
                std::cout << "Port number: " << port << std::endl;
                break;
            default:
                break;
        }
    }

    Logger::setLogFileName(logPath);//设置日志文件路径
    EventLoop mainLoop;//创建事件循环对象
    Server myWebServer(&mainLoop, thread_number, port);//创建基于http的web服务器对象
    myWebServer.startup(&port);//启动服务器
    mainLoop.loop(); //进入主事件循环,程序会在此处等待事件并处理
    return 0;
}