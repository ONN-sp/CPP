#ifndef LOGSTREAM_H
#define  LOGGINGSTREAM_H

#include <string>
#include <cstring>
#include <algorithm>
#include "../NonCopyAble.h"

namespace tiny_muduo{
    const int kSmallSize = 4096;//小缓冲区大小
    const int kLargeSize = 4096*1000;//大缓冲区大小
    static const int kNum2MaxStringLength = 48;//最大的数字转换为字符串的长度
    static const char digits[] = {'9', '8', '7', '6', '5', '4', '3', '2', '1', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};//用于快速转换数字字符 
    // GeneralTemplate 类用于存储数据指针和长度
    class GeneralTemplate : public NonCopyAble{
        public:
            GeneralTemplate();
            explicit GeneralTemplate(const char*, int);
        const char* data_;
        int len_;     
    };
    //FixedBuffer模板,管理固定大小的缓冲区
    template <int SIZE>
    class FixedBuffer : public NonCopyAble{
        public:
            FixedBuffer() : cur_(buf_) {
                SetCookie(CookieStart); 
            }
            ~FixedBuffer() {
                SetCookie(CookieEnd);
            }
            static void CookieStart();// 静态函数，缓冲区开始使用时的回调函数，可以自定义 比如记录日志
            static void CookieEnd();// 静态函数，缓冲区结束使用时的回调函数，可以自定义 比如清理资源
            void Append(const char* input_data, int length){//将数据加到缓冲区  length表示待加数据的长度
                if(writeablebytes(0 < length)){
                    length = writaeblebytes();//如果数据超过可写字节数,则截断
                }
                memcpy(cur_, input_data, length);//将数据复制到当前指针位置
                cur_ += length;//更新当前指针位置
            }
            void SetBufferZero(){//缓冲区置零
                memset(buf_, '\0', sizeof(buf_));//将缓冲区置零
                cur_ = buf_;//重置当前指针位置位缓冲区起始位置
            }
            void SetCookie(void(*cookie)()){//设置cookie函数指针
                cookie_ = cookie;
            }
            void Add(int length){//增加当前指针位置,表示写入了length长数据
                cur_ += length;
            }
            const char* data() const {return buf_;}//获取缓冲区数据指针
            int len() const {return static_cast<int>(cur_ - buf_);}//获取缓冲区长度
            int writeablebytes() const {return static_cast<int>(end() - cur_);}//获取可写字节数
            const char* current() const {return cur_;}//获取当前指针位置
            const char* end() const {return buf_+sizeof(buf_);}//获取缓冲区结束位置指针
        private:
            void (*cookie_)();//cookie函数指针
            char buf_[SIZE];//缓冲区数组,大小由模板参数决定
            char* cur_;//当前指针位置
    };
    //Logstream类,用于流式日志输出
    class LogStream{
        public:
            using Buffer = FixedBuffer<kSmallSize>;//使用小缓冲区
            using Self = LogStream;//自定义类型别名
            LogStream() = default;
            ~LogStream() = default;
            //返回缓冲区的引用
            Buffer& buffer(){return buffer_;}
            //将整数格式化为字符串 并加入缓冲区
            template <typename T>
            void FormatInteger(T num){
              if(buffer_.writeablebytes() >= kNum2MaxStringLength){//检查缓冲区是否有足够的空间容纳数字字符串
                char* buf = buffer_.current();//获取当前缓冲区位置
                char* now = buf;
                const char* zero = digits + 9;//数字转换表的首地址
                bool negative = num < 0;//检查数字是否为负
                //转换整数为字符串  除余旋转法 从个位开始
                do{
                    int remainder = static_cast<int>(num%10);//获取个位值
                    *(now++) = zero[remainder];//根据数字转换表digits+9找到对应的字符
                    num /= 10;//去掉最后一位
                }while(num!=0)
                if(negative)//如果为负数,则添加负号
                    *(now++) = '-';
                *now = '\0';
                std::reverse(buf, now);
                buffer_.Add(static_cast<int>(now-buf));//更新缓冲区当前指针位置
              }            
            }
            // 重载操作符<<，支持各种数据类型
            Self& operator<<(short num) {//将short类型转换为int类型再处理
                return (*this) << static_cast<int>(num);
            }

            Self& operator<<(unsigned short num) { // 将unsigned short类型转换为unsigned int类型再处理
                return (*this) << static_cast<unsigned int>(num);
            }

            Self& operator<<(int num) {// 格式化整数并追加到缓冲区
                FormatInteger(num);
                return *this;
            }

            Self& operator<<(unsigned int num) { // 格式化无符号整数并追加到缓冲区
                FormatInteger(num);
                return *this;
            }

            Self& operator<<(long num) {// 格式化长整数并追加到缓冲区
                FormatInteger(num);
                return *this;
            }

            Self& operator<<(unsigned long num) {// 格式化无符号长整数并追加到缓冲区
                FormatInteger(num);
                return *this;
            }

            Self& operator<<(long long num) {// 格式化长长整数并追加到缓冲区
                FormatInteger(num);
                return *this;
            }

            Self& operator<<(unsigned long long num) {// 格式化无符号长长整数并追加到缓冲区
                FormatInteger(num);
                return *this;
            }

            Self& operator<<(const float& num) {// 将float类型转换为double类型再处理
                return (*this) << static_cast<double>(num);
            }

            Self& operator<<(const double& num) {
                char buf[32]; // 存储浮点数转换后的字符串
                int len = snprintf(buf, sizeof(buf), "%g", num); // 使用snprintf转换浮点数->字符串
                buffer_.Append(buf, len); // 追加到缓冲区
                return *this;
            }

            Self& operator<<(bool boolean) {// 将bool类型转换为整数1或0再处理
                return (*this) << (boolean ? 1 : 0);
            }

            Self& operator<<(char chr) {// 追加单个字符到缓冲区
                buffer_.Append(&chr, 1);
                return *this;
            }

            Self& operator<<(const void* data) {// 将指针转换为字符指针处理
                return (*this) << static_cast<const char*>(data);
            }

            Self& operator<<(const char* data) {// 追加字符数组到缓冲区
                buffer_.Append(data, static_cast<int>(strlen(data)));
                return *this;
            }

            Self& operator<<(const GeneralTemplate& source) {// 追加GeneralTemplate的内容到缓冲区
                buffer_.Append(source.data_, source.len_);
                return *this;
            }

            Self& operator<<(const std::string& str) {// 追加std::string的内容到缓冲区
                buffer_.Append(str.data(), static_cast<int>(str.size()));
                return *this;
            }

            private:
                Buffer buffer_;//一个FixedBuffer实例
    };
}

#endif