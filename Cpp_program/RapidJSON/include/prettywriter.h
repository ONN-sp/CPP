#ifndef RAPIDJSON_PRETTYWRITER_H_
#define RAPIDJSON_PRETTYWRITER_H_

#include "writer.h"

namespace RAPIDJSON{
enum PrettyFormatOptions{
    kFormatDefault = 0,// 默认的格式化
    kFormatSingleLineArray = 1// 数组显示在单行上  对对象无用
};
/**
 * @brief 定义一个带有缩进和空格的JSON编写器  它就是在writer的基础上多了格式化的美化输出,使其更加易读(如:增加了缩进(表示层级关系)、换行等)
 * 
 * @tparam OutputStream 
 * @tparam SourceEncoding 
 * @tparam TargetEncoding 
 * @tparam StackAllocator 
 * @tparam writeFlags 
 */
template<typename OutputStream, typename SourceEncoding=UTF8<>, typename TargetEncoding = UTF8<>, typename StackAllocator = CrtAllocator, unsigned writeFlags = kWriteDefaultFlags>
class PrettyWriter : public Writer<OutputStream, SourceEncoding, TargetEncoding, StackAllocator, writeFlags>{
    public:
        typedef Writer<OutputStream, SourceEncoding, TargetEncoding, StackAllocator, writeFlags> Base;
        typedef typename Base::Ch Ch;
        /**
         * @brief 构建一个带有缩进和空格和换行的格式化处理的JSON编写器
         * 
         * @param os 
         * @param allocator 
         * @param levelDepth 
         */
        explicit PrettyWriter(OutputStream& os, StackAllocator* allocator=nullptr, size_t levelDepth = Base::kDefaultLevelDepth)
            : Base(os, allocator, levelDepth),
              indentChar_(' '),// 缩进使用空格(而不是Tab等)
              indentCharCount_(4),// 默认每级往里面缩进4个字符
              formatOptions_(kFormatDefault)
            {}
        explicit PrettyWriter(StackAllocator* allocator=0, size_t levelDepth = Base::kDefaultLevelDepth)
            : Base(allocator, levelDepth),
              indentChar_(' '),// 缩进使用空格(而不是Tab等)
              indentCharCount_(4),// 默认每级往里面缩进4个字符
              formatOptions_(kFormatDefault)
            {}
        #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
            PrettyWriter(PrettyWriter&& rhs)
                : Base(std::forward<PrettyWriter>(rhs)),
                  indentChar_(rhs.indentChar_),// 缩进使用空格(而不是Tab等)
                  indentCharCount_(rhs.indentCharCount_),// 默认每级往里面缩进4个字符
                  formatOptions_(rhs.formatOptions_)
                {}
        #endif
        /**
         * @brief 设置缩进的字符和缩进字符数
         * 
         * @param indentChar 
         * @param indentCharCount 
         * @return PrettyWriter& 
         */
        PrettyWriter& SetIndent(Ch indentChar, unsigned indentCharCount){
            RAPIDJSON_ASSERT(indentChar==' '||indentChar=='\t'||indentChar=='\n'||indentChar=='\r');// 确保缩进字符是空格、制表符、换行符或回车符
            indentChar_ = indentChar;
            indentCharCount_ = indentCharCount;
            return *this;
        }
        /**
         * @brief 设置格式化选项,例如数组写成单行的这种选项
         * 
         * @param options 
         * @return PrettyWriter& 
         */
        PrettyWriter& SetFormatOptions(PrettyFormatOptions options){
            formatOptions_ = options;
            return *this;
        }
        // 实际上和writer.h中的一样,只是一个是Prefix一个是PrettyPrefix  实现Handler接口的成员函数
        // 注意这里的类型其实是要写一个具体的值到输出流,即先进行前缀处理,然后写入相应的int、string、null、double值等到输出流
        bool Null()                 { PrettyPrefix(kNullType);   return Base::EndValue(Base::WriteNull()); }
        bool Bool(bool b)           { PrettyPrefix(b ? kTrueType : kFalseType); return Base::EndValue(Base::WriteBool(b)); }
        bool Int(int i)             { PrettyPrefix(kNumberType); return Base::EndValue(Base::WriteInt(i)); }
        bool Uint(unsigned u)       { PrettyPrefix(kNumberType); return Base::EndValue(Base::WriteUint(u)); }
        bool Int64(int64_t i64)     { PrettyPrefix(kNumberType); return Base::EndValue(Base::WriteInt64(i64)); }
        bool Uint64(uint64_t u64)   { PrettyPrefix(kNumberType); return Base::EndValue(Base::WriteUint64(u64));  }
        bool Double(double d)       { PrettyPrefix(kNumberType); return Base::EndValue(Base::WriteDouble(d)); }
        bool RawNumber(const Ch* str, SizeType length, bool copy=false){
            RAPIDJSON_ASSERT(str!=0);
            (void)copy;
            PrettyPrefix(kNumberType);
            return Base::EndValue(Base::WriteString(str, length));
        }
        bool String(const Ch* str, SizeType length, bool copy = false){
            RAPIDJSON_ASSERT(str!=0);
            (void)copy;
            PrettyPrefix(kStringType);
            return Base::EndValue(Base::WriteString(str, length));
        }
        #if RAPIDJSON_HAS_STDSTRING 
            bool String(const std::basic_string<Ch>& str){
                return String(str.data(), SizeType(str.size()));
            }
        #endif
        /**
         * @brief 开始对象  PrettyPrefix+WriteStartObject
         * 
         * @return true 
         * @return false 
         */
        bool StartObject(){
            PrettyPrefix(kObjectType);
            new (Base::level_stack_.template Push<typename Base::Level>()) typename Base::Level(false);
            return Base::WriteStartObject();
        }
        bool Key(const Ch* str, SizeType length, bool copy = false){
                return String(str, length, copy);// 写入键
        }
        #if RAPIDJSON_HAS_STDSTRING
            bool Key(const std::basic_string<Ch>& str){
                return Key(str.data(), SizeType(str.size()));
            }
        #endif
        /**
         * @brief 结束对象 
         * 
         * @param memberCount 
         * @return true 
         * @return false 
         */
        bool EndObject(SizeType memberCount=0){
            (void)memberCount;
            RAPIDJSON_ASSERT(Base::level_stack_.GetSize() >= sizeof(typename Base::Level));// 确保当前栈的大小>=Level大小,即确保当前栈至少包含一个完整的Level(false)结构(对于对象来说)
            RAPIDJSON_ASSERT(!Base::level_stack_.template Top<typename Base::Level>()->inArray);// 确保不在数组内
            RAPIDJSON_ASSERT(0==Base::level_stack_.template Top<typename Base::Level>()->valueCount % 2);// 确保对象的键值对数量为偶数,即有一个key就有一个value对应
            bool empty = Base::level_stack_.template Pop<typename Base::Level>(1)->valueCount == 0;// 弹出当前对象的层级,并检查它是否为空
            if(!empty){// 对象不空,写入换行符和缩进
                Base::os_->Put('\n');// 写入换行符
                WriteIndent();// 写入缩进
            }
            bool ret = Base::EndValue(Base::WriteEndObject());// 结束JSON对象,并处理返回值
            return ret;
        }
        /**
         * @brief 开始数组 PrettyPrefix+WriteStartArray
         * 
         * @return true 
         * @return false 
         */
        bool StartArray(){
            PrettyPrefix(kArrayType);
            new (Base::level_stack_.template Push<typename Base::Level>()) typename Base::Level(true);// true对应数组
            return Base::WriteStartArray();
        }
        /**
         * @brief 结束数组
         * 
         * @param memberCount 
         * @return true 
         * @return false 
         */
        bool EndArray(SizeType memberCount=0){
            (void)memberCount;
            RAPIDJSON_ASSERT(Base::level_stack_.GetSize() >= sizeof(typename Base::Level));// 确保当前栈的大小>=Level大小,即确保当前栈至少包含一个完整的Level(true)结构(对于数组来说)
            RAPIDJSON_ASSERT(Base::level_stack_.template Top<typename Base::Level>()->inArray);// 确保不在数组内
            bool empty = Base::level_stack_.template Pop<typename Base::Level>(1)->valueCount == 0;// 弹出当前对象的层级,并检查它是否为空
            if(!empty && !(formatOptions_&kFormatSingleLineArray)){// 数组不空&&非单行显示数据,写入换行符和缩进
                Base::os_->Put('\n');// 写入换行符
                WriteIndent();// 写入缩进
            }
            bool ret = Base::EndValue(Base::WriteEndArray());// 结束JSON对象,并处理返回值
            return ret;
        }
        // 以下是为了兼容不支持C++11中的std::string类型而设计的函数重载
        bool String(const Ch* str) {return String(str, internal::StrLen(str));}
        bool Key(const Ch* str) {return Key(str, internal::StrLen(str));}
        /**
         * @brief 将已经序列化的JSON字符串(已经按照JSON格式编码过的数据)作为原始值直接写入输出流
         * 
         * @param json 
         * @param length 
         * @param type 
         * @return true 
         * @return false 
         */
        bool RawValue(const Ch* json, size_t length, Type type){
            RAPIDJSON_ASSERT(json!=nullptr);
            PrettyPrefix(type);
            return Base::EndValue(Base::WriteRawValue(json, length));
        }
    protected:
        /**
         * @brief 在JSON序列化时处理元素的前缀,以确保格式正确  相比于Writer中的Prefix多了格式化处理
         * 
         * @param type 
         */
        void PrettyPrefix(Type type){
            (void)type;
            if(Base::level_stack_.GetSize()!=0){// 表示当前正在处理的元素不是根元素 
                typename Base::Level* level = Base::level_stack_.template Top<typename Base::Level>();
            if(level->inArray){// 当前层级为数组
                if(level->valueCount>0){// 当前层级是数组且非第一个元素,则写入新值前要添加逗号
                    Base::os_->Put(',');
                    if(formatOptions_&kFormatSingleLineArray)// 设置了单行数组格式化选项,则在逗号后添加空格
                        Base::os_->Put(' ');
                    else{// 没有设置单行格式选项,则换行+缩进
                        Base::os_->Put('\n');
                        WriteIndent();
                    }
                }
                else{// 当前层没有值,则添加第一个元素前先换行再缩进处理
                    Base::os_->Put('\n');
                   WriteIndent(); 
                }
            }
            else{
                if(level->valueCount>0){
                    if(level->valueCount % 2 ==0){// 如果值计数为偶数,表示当前是对象的一个键值对的开始,则添加逗号和换行和缩进
                        Base::os_->Put(',');
                        Base::os_->Put('\n');
                        WriteIndent();
                    }
                    else{// 如果值计数为奇数,表示当前是键,则添加冒号和空格
                        Base::os_->Put(':');
                        Base::os_->Put(' ');
                    }
                }
                else{// 第一个元素要先换行再缩进  此时肯定是处理的键
                    Base::os_->Put('\n');// 如果是第一个元素,则直接换行
                    WriteIndent();
                }    
            }
            if(!level->inArray&&level->valueCount%2==0)// 如果当前不是数组,并且值计数为偶数,确保类型是字符串(表示对象的键)
                RAPIDJSON_ASSERT(type==kStringType);
            level->valueCount++;
            }
            else{// 如果在根元素处,要确保只有一个根元素
                RAPIDJSON_ASSERT(!Base::hasRoot_);// 只能有一个根元素
                Base::hasRoot_ = true;
            }
        }
        /**
         * @brief 根据当前层级计算当前若添加元素需要缩进多少
         * 
         */
        void WriteIndent(){
            // GetSize():返回栈的当前大小,若一个栈元素占4字节,那么若栈有两个元素,那么GetSize()返回的是8字节
            size_t count = (Base::level_stack_.GetSize() / sizeof(typename Base::Level))*indentCharCount_;// 计算当前层级需要添加的缩进字符数量
            PutN(*Base::os_, static_cast<typename OutputStream::Ch>(indentCharCount_), count);
        }
        Ch indentChar_;// 用于存储缩进时使用的字符.可以是空格、制表符或其它空白字符
        unsigned indentCharCount_;// 指定每个缩进级别的字符数量.如果indentChar_是空格字符,且indentCharCount_为4,那么每个缩进级别将使用4个空格
        PrettyFormatOptions formatOptions_;// 格式化类型,可以决定是否将数组格式化为单行
private:
    PrettyWriter(const PrettyWriter&) = delete;
    PrettyWriter& operator=(const PrettyWriter&) = delete;
};
}
#endif