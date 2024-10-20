#ifndef RAPIDJSON_ENCODINGS_H_
#define RAPIDJSON_ENCODINGS_H_

#include "rapidjson.h"

namespace RAPIDJSON{
    template<typename CharType=char>
    struct UTF8{
        typedef CharType Ch;
        enum { supportUnicode = 1};// 支持Unicode
        /**
         * @brief 将一个Unicode代码点编码为UTF8字节序列,并将结果写入输出流os中
         * 代码点范围从 0x0 到 0x10FFFF,覆盖了大部分Unicode字符
         * @tparam OutputStream 
         * @param os 
         * @param codepoint 
         */
        template<typename OutputStream>
        static void Encode(OutputStream& os, unsigned codepoint){// 代码点:字符在字符集Unicode中对应的唯一数字标识符
            if (codepoint <= 0x7F) // 一个字节(ASCII)则直接写入输出流os中
                os.Put(static_cast<Ch>(codepoint & 0xFF));
            else if(codepoint <= 0x7FF){// 两字节字符编码处理
                os.Put(static_cast<Ch>(0xC0 | ((codepoint >> 6) & 0xFF)));
                os.Put(static_cast<Ch>(0x80 | ((codepoint & 0x3F))));
            }
            else if(codepoint <= 0xFFFF){// 三字节字符处理
                os.Put(static_cast<Ch>(0xE0 | ((codepoint >> 12) & 0xFF)));
                os.Put(static_cast<Ch>(0x80 | ((codepoint >> 6) & 0x3F)));
                os.Put(static_cast<Ch>(0x80 | (codepoint & 0x3F)));
            }
            else{// 四字节字符处理
                RAPIDJSON_ASSERT(codepoint <= 0x10FFFF);
                os.Put(static_cast<Ch>(0xF0 | (codepoint >> 18)));
                os.Put(static_cast<Ch>(0x80 | ((codepoint >> 12) & 0x3F)));
                os.Put(static_cast<Ch>(0x80 | ((codepoint >> 6) & 0x3F)));
                os.Put(static_cast<Ch>(0x80 | (codepoint & 0x3F)));
            }
        }
        /**
         * @brief 不进行安全检查的Encode() Unicode代码点->UTF8字节序列
         * 
         * @tparam OutputStream 
         * @param os 
         * @param codepoint 
         */
        template<typename OutputStream>
        static void EncodeUnsafe(OutputStream& os, unsigned codepoint){
            if(codepoint <= 0x7F)
                PutUnsafe(os, static_cast<Ch>(codepoint & 0xFF));
            else if(codepoint <= 0x7FF){
                PutUnsafe(os, static_cast<Ch>(0xC0 | ((codepoint >> 6) & 0xFF)));
                PutUnsafe(os, static_cast<Ch>(0x80 | ((codepoint & 0x3F))));
            }
            else if (codepoint <= 0xFFFF) {
            PutUnsafe(os, static_cast<Ch>(0xE0 | ((codepoint >> 12) & 0xFF)));
            PutUnsafe(os, static_cast<Ch>(0x80 | ((codepoint >> 6) & 0x3F)));
            PutUnsafe(os, static_cast<Ch>(0x80 | (codepoint & 0x3F)));
            }
            else {
            RAPIDJSON_ASSERT(codepoint <= 0x10FFFF);
            PutUnsafe(os, static_cast<Ch>(0xF0 | ((codepoint >> 18) & 0xFF)));
            PutUnsafe(os, static_cast<Ch>(0x80 | ((codepoint >> 12) & 0x3F)));
            PutUnsafe(os, static_cast<Ch>(0x80 | ((codepoint >> 6) & 0x3F)));
            PutUnsafe(os, static_cast<Ch>(0x80 | (codepoint & 0x3F)));
            }
        }
        /**
         * @brief 从输入流中解码UTF8字符  并将其转换为Unicode代码点
         * 
         * @tparam InputStream 
         * @param is 
         * @param codepoint 
         * @return true 
         * @return false 
         */
        template<typename InputStream>
        static bool Decode(InputStream& is, unsigned* codepoint){
            // 从输入流中获取下一个字符c,并更新代码点codepoint
            #define RAPIDJSON_COPY() c = is.Take();*codepoint = (*codepoint << 6) | (static_cast<unsigned char>(c) & 0x3Fu)
            // 检查字符的类型是否符合特定的掩码条件
            #define RAPIDJSON_TRANS(mask) result &= ((GetRange(static_cast<unsigned char>(c)) & mask) != 0)
            // 执行RAPIDJSON_TAIL()就执行了RAPIDJSON_COPY()和RAPIDJSON_TRANS(0x70)
            #define RAPIDJSON_TAIL() RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x70)
            typename InputStream::Ch c = is.Take();
            if(!(c&0x80)){// 如果c的最高位为0(c&0x80==0),此时表示这是一个单字节字符(ASCII),则直接将其转换为代码点并返回true
                *codepoint = static_cast<unsigned char>(c);
                return true;
            }
            unsigned char type = GetRange(static_cast<unsigned char>(c));// type值表示UTF8字符的字节数
            if(type>=32)// 不合法
                *codepoint = 0;// 重置代码点
            else// 根据字符c的类型初始化codepoint
                *codepoint = (0xFFu >> type) & static_cast<unsigned char>(c);
            bool result = true;
            switch(type){// 根据type值,执行不同的解码过程  TODO:解码过程不太理解
                case 2: RAPIDJSON_TAIL(); return result;// 2字节
                case 3: RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
                case 4: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x50); RAPIDJSON_TAIL(); return result;
                case 5: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x10); RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
                case 6: RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
                case 10: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x20); RAPIDJSON_TAIL(); return result;
                case 11: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x60); RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
                default: return false;
            }
            // 清除宏定义以避免后续的冲突
            #undef RAPIDJSON_COPY
            #undef RAPIDJSON_TRANS
            #undef RAPIDJSON_TAIL
        }
        /**
         * @brief 验证一个输入流中的UTF8字符串是否符合UTF8编码规范,并将验证为有效的字节写入输出流
         * 
         * @tparam InputStream 
         * @tparam OutputStream 
         * @param is 
         * @param os 
         * @return true 
         * @return false 
         */
        template <typename InputStream, typename OutputStream>
        static bool Validate(InputStream& is, OutputStream& os) {
            #define RAPIDJSON_COPY() os.Put(c = is.Take())
            #define RAPIDJSON_TRANS(mask) result &= ((GetRange(static_cast<unsigned char>(c)) & mask) != 0)
            #define RAPIDJSON_TAIL() RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x70)
            Ch c;
            RAPIDJSON_COPY();
            if (!(c & 0x80))
                return true;

            bool result = true;
            switch (GetRange(static_cast<unsigned char>(c))) {
            case 2: RAPIDJSON_TAIL(); return result;
            case 3: RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
            case 4: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x50); RAPIDJSON_TAIL(); return result;
            case 5: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x10); RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
            case 6: RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
            case 10: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x20); RAPIDJSON_TAIL(); return result;
            case 11: RAPIDJSON_COPY(); RAPIDJSON_TRANS(0x60); RAPIDJSON_TAIL(); RAPIDJSON_TAIL(); return result;
            default: return false;
            }
            #undef RAPIDJSON_COPY
            #undef RAPIDJSON_TRANS
            #undef RAPIDJSON_TAIL
        }
        /**
         * @brief 根据输入的字符c确定其在UTF8编码中的类型(单字节、多字节、无效类型等等)
         * 
         * @param c 
         * @return unsigned char 
         */
        static unsigned char GetRange(unsigned char c) {
            // 根据DFA进行UTF8解码 判断输入的字节序列是否符合UTF8规范
            // 新映射1->0x10,7->0x20,9->0x40使得与操作可以测试多种类型
            // C='A'  type[65]=0
            static const unsigned char type[] = {// 映射表  8,9,10,11表示无效字符或错误情况  0:单字节字符(ASCII)  0x10:多字节字符的起始字节  0x20:后续字节(在多字节序列中)
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
                0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
                0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
                0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
                8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,
            };
            return type[c];
        }
        /**
         * @brief 从输入流中读取字节,检查是否是UTF8的BOM,即0xEF 0xBB 0xBF
         * 如果输入流符合BOM模式,那么会读取到0xEF 0xBB 0xBF三个字节后,再将后一个字节返回;
         * 否则就是返回不符合BOM字节处的字节
         * @tparam InputByteStream 
         * @param is 
         * @return CharType 
         */
        template <typename InputByteStream>
        static CharType TakeBOM(InputByteStream& is) {
            RAPIDJSON_STATIC_ASSERT(sizeof(typename InputByteStream::Ch) == 1);
            typename InputByteStream::Ch c = Take(is);
            if (static_cast<unsigned char>(c) != 0xEFu) return c;
            c = is.Take();
            if (static_cast<unsigned char>(c) != 0xBBu) return c;
            c = is.Take();
            if (static_cast<unsigned char>(c) != 0xBFu) return c;
            c = is.Take();
            return c;
        }
        /**
         * @brief 从输入流中读取一个字节并返回
         * 
         * @tparam InputByteStream 
         * @param is 
         * @return Ch 
         */
        template <typename InputByteStream>
        static Ch Take(InputByteStream& is) {
            RAPIDJSON_STATIC_ASSERT(sizeof(typename InputByteStream::Ch) == 1);
            return static_cast<Ch>(is.Take());
        }
        /**
         * @brief 向输出流写入UTF8的BOM  即分别写入三个字节0xEF,0xBB,0xBF
         * 
         * @tparam OutputByteStream 
         * @param os 
         */
        template <typename OutputByteStream>
        static void PutBOM(OutputByteStream& os) {
            RAPIDJSON_STATIC_ASSERT(sizeof(typename OutputByteStream::Ch) == 1);
            os.Put(static_cast<typename OutputByteStream::Ch>(0xEFu));
            os.Put(static_cast<typename OutputByteStream::Ch>(0xBBu));
            os.Put(static_cast<typename OutputByteStream::Ch>(0xBFu));
        }
        /**
         * @brief 向输出流写一个字符
         * 
         * @tparam OutputByteStream 
         * @param os 
         * @param c 
         */
        template <typename OutputByteStream>
        static void Put(OutputByteStream& os, Ch c) {
            RAPIDJSON_STATIC_ASSERT(sizeof(typename OutputByteStream::Ch) == 1);
            os.Put(static_cast<typename OutputByteStream::Ch>(c));
        }   
    };
}
#endif