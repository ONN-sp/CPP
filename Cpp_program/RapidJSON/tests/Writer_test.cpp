#include <iostream>
#include "../include/writer.h"
#include "../include/stringbuffer.h"

using namespace RAPIDJSON;

int main() {
    // 创建一个 RapidJSON StringBuffer 用于存储生成的 JSON 字符串
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    // 开始写入 JSON 对象
    writer.StartObject(); // 开始一个对象
    // 写入键值对
    writer.Key("name");
    writer.String("Alice");
    writer.Key("age");
    writer.Int(30);
    // 写入一个数组
    writer.Key("hobbies");
    writer.StartArray(); // 开始一个数组
    writer.String("Reading");
    writer.String("Traveling");
    writer.String("Cooking");
    writer.EndArray(); // 结束数组
    writer.EndObject(); // 结束对象
    // 输出生成的 JSON 字符串
    std::cout << "Generated JSON: " << buffer.GetString() << std::endl;
    return 0;
}