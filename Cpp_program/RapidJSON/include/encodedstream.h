// 仅用于UTF8
#ifndef RAPIDJSON_ENCODEDSTREAM_H_
#define RAPIDJSON_ENCODEDSTREAM_H_

#include "stream.h"
#include "memorystream.h"

namespace RAPIDJSON{
    /**
     * @brief 将一个输入字节流和特定编码类型(本项目只涉及UTF8)绑定在一起,并调用相应的编码对应的方法函数
     * 
     * @tparam Encoding 
     * @tparam InputByteStream 
     */
    template<typename Encoding, typename InputByteStream>
    class EncodedInputStream{
        RAPIDJSON_STATIC_ASSERT(sizeof(typename InputByteStream::Ch)==1);// 编译时断言,确保输入字节流中的字符类型大小为1字节
        public:
            typedef typedef Encoding::Ch Ch;
            EncodedInputStream(InputByteStream& is) : is_(is){
                current_ = Encoding::TakeBOM(is_);// 读取并处理(处理就是几次Take())可能存在的BOM
            }
            Ch Peek() const {return current_;}
            Ch Take() {
                Ch c = current_;
                current_ = Encoding::Take(is_);
                return c;
            }
            size_t Tell() const {return is_.Tell();}
            // 以下函数未实现，调用时会触发断言错误
            void Put(Ch) { RAPIDJSON_ASSERT(false); }
            void Flush() { RAPIDJSON_ASSERT(false); } 
            Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0;}
        private:
            // 禁止拷贝构造和赋值操作
            EncodedInputStream(const EncodedInputStream&) = delete;
            EncodedInputStream& operator=(const EncodedInputStream&) = delete;
            InputByteStream& is_;// 输入字节流
            Ch current_;// 当前字符
    };
    /**
     * @brief 上述类的特化版本  专门处理内存中且编码为Encoding=UTF8的数据的特化版本
     * 
     * @param  
     */
    template<>
    class EncodedInputStream<UTF8<>, MemoryStream>{
        public:
            typedef UTF8<>::Ch Ch;
            EncodedInputStream(MemoryStream& is) : is_(is){
                // 检测BOM
                if(static_cast<unsigned char>(is_.Peek())==0xEFu)
                    is_.Take();
                if(static_cast<unsigned char>(is_.Peek())==0xBBu)
                    is_.Take();
                if(static_cast<unsigned char>(is_.Peek())==0xBFu)
                    is_.Take();
            }
            Ch Peek() const {return is_.Peek();}
            Ch Take() {is_.Take();}
            size_t Tell() const {return is_.Tell();}
            // 以下函数未实现，调用时会触发断言错误
            void Put(Ch) { RAPIDJSON_ASSERT(false); }
            void Flush() { RAPIDJSON_ASSERT(false); } 
            Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0;} 
        private:
            // 禁止拷贝构造和赋值操作
            EncodedInputStream(const EncodedInputStream&) = delete;
            EncodedInputStream& operator=(const EncodedInputStream&) = delete;
            MemoryStream& is_;// 输入字节流
    };
    /**
     * @brief 输出字节流的包装器,并使用静态绑定的编码类型
     * 
     * @tparam Encoding 
     * @tparam OutputByteStream 
     */
    template<typename Encoding, typename OutputByteStream>
    class EncodedOutputStream{
        RAPIDJSON_STATIC_ASSERT(sizeof(typename OutputByteStream::Ch)==1);
        public:
            typedef typename Encoding::Ch Ch;
            EncodedOutputStream(OutputByteStream& os, bool putBOM=true) : os_(os){
                // 写入BOM
                if(putBOM)
                    Encoding::PutBOM(os_);
            }
            void Put(Ch c) {Encoding::Put(os_, c);}
            void Flush() {oos_.Flush();}
            // 以下函数未实现，调用时会触发断言错误
            Ch Peek() const { RAPIDJSON_ASSERT(false); return 0;}
            Ch Take() { RAPIDJSON_ASSERT(false); return 0;}
            size_t Tell() const { RAPIDJSON_ASSERT(false);  return 0; }
            Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
            size_t PutEnd(Ch*) { RAPIDJSON_ASSERT(false); return 0; }
        private:
             // 禁止拷贝构造和赋值操作
            EncodedOutputStream(const EncodedOutputStream&) = delete;
            EncodedOutputStream& operator=(const EncodedOutputStream&) = delete;
            OutputByteStream& os_;// 输出字节流
    };
    #define RAPIDJSON_ENCODINGS_FUNC(x) UTF8<Ch>::x
}
#endif