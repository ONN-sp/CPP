#ifndef RAPIDJSON_CURSORSTREAMWRAPPER_H_
#define RAPIDJSON_CURSORSTREAMWRAPPER_H_

#include "stream.h"

namespace RAPIDJSON{
    template <typename InputStream, typename Encoding=UTF8<>>
    class CursorStreamWrapper : public GenericStreamWrapper<InputStream, Encoding>{
        public:
            typedef typename Encoding::Ch Ch;
            CursorStreamWrapper(InputStream& is)
                : GenericStreamWrapper<InputStream, Encoding>(is),
                  line_(1),
                  col_(0)
                {}
            /**
             * @brief Take():取出当前缓冲区中读取位置的字符,并从下一个字符重新继续读取
             * 在取出当前缓冲区中读取位置的字符时更新行号和列号
             * @return Ch 
             */
            Ch Take(){
                Ch ch = this->is_.Take();
                if(ch=='\n'){
                    line_++;
                    col_ = 0;
                }
                else
                    col_++;
                return ch;
            }
            /**
             * @brief 获取错误所在的行号
             * 
             * @return size_t 
             */
            size_t GetLine() const {return line_;}
            /**
             * @brief 获取错误所在的列号
             * 
             * @return size_t 
             */
            size_t GetColumn() const {return col_;}
        private:
            size_t line_;// 当前行数
            size_t col_;// 当前列数
    };
}
#endif