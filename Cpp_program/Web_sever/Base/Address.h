// Address类在的作用是简化和抽象IP地址和端口的管理与使用,它提供了一个方便的方式来表示和操作网络终端
#ifndef ADDRESS_H
#define ADDRESS_H

#include <stdexcept>  // 引入异常库以便处理非法端口
#include <string>
#include <netinet/in.h>

namespace tiny_muduo{
    class Address{
        public:
            explicit Address(const std::string& port)
                : ip_("0.0.0.0"),
                  port_(std::stoi(port)){
                    if(port_<0 || port_>65535)
                        throw std::out_of_range("The port value in invalid");
                  }
            explicit Address(int port)
                : ip_("0.0.0.0"),
                  port_(port){
                    if(port_<0 || port_>65535)
                        throw std::out_of_range("The port value in invalid");
                  }
            Address(const std::string& ip="127.0.0.1", const std::string& port="80")
                : ip_(ip),
                  port_(std::stoi(port)){
                    if(port_<0 || port_>65535)
                        throw std::out_of_range("The port value in invalid");
                  }
            ~Address() = default;
            // 获取IP地址
            const std::string& IP() const {
                return ip_;
            }
            // 获取端口号
            int Port() const {
                return port_;
            }
            // 返回IP地址和端口号的字符串表示,"ip:port"
            std::string IpPortToString() const {
                return ip_+":"+std::to_string(port_);
            }
            // 设置IPV4的地址结构
            void SetSockAddrInet4(const struct sockaddr_in& addr){
                addr_ = addr;
            }
        private:
            std::string ip_; // IP地址
            int port_; // 端口号
            sockaddr_in addr_;// 结构体地址
    };
}

#endif