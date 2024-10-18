#ifndef RAPIDJSON_OSTREAMWRAPPER_H_
#define RAPIDJSON_OSTREAMWRAPPER_H_

#include "stream.h"
#include <iosfwd>// 引入C++标准库中的输入输出流的前置声明

namespace RAPIDJSON{
    template <typename StreamType>
    class BasicOStreamWrapper{
        public:
            typedef typename StreamType::char_type Ch; // 定义字符类型,来自StreamType的char_type
            /**
             * @brief 传入一个要写入的输出流
             * 
             * @param stream 
             */
            BasicOStreamWrapper(StreamType& stream) : stream_(stream) {}
            /**
             * @brief 向输出流写一个字符
             * 
             * @param c 
             */
            void Put(Ch c){
                stream_.put(c);// 调用流stream_的put()函数写入字符c
            }
            /**
             * @brief 刷新输出流  缓冲区内容写到输出流中去
             * 
             */
            void Flush(){
                stream_.flush();// 刷新流,确保输出流stream_的输出缓冲区的内容被写到目标输出中去
            }
            //! 未实现的函数，RapidJSON不需要这些方法
            char Peek() const { RAPIDJSON_ASSERT(false); return 0; }
            char Take() { RAPIDJSON_ASSERT(false); return 0; }
            size_t Tell() const { RAPIDJSON_ASSERT(false); return 0; }
            char* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
            size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }
        private:
            // 禁用拷贝构造函数和拷贝赋值运算符
            BasicOStreamWrapper(const BasicOStreamWrapper&) = delete;
            BasicOStreamWrapper& operator=(const BasicOStreamWrapper&)= delete;
            StreamType& stream_;// 输出流
    };
    typedef BasicOStreamWrapper<std::ostream> OStreamWrapper;// 包装std::ostream
    typedef BasicOStreamWrapper<std::wostream> WOStreamWrapper;// 包装std::wostream(宽字符流)
}
#endif