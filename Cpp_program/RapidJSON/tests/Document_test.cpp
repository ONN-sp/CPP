#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

using namespace RAPIDJSON;
using namespace std;

int main() {
    // 1. 创建一个空的 GenericDocument
    Document doc;

    // 2. 解析一个 JSON 字符串
  const char* json = R"({
        "name": "John",
        "age": 30,
        "isStudent": false,
        "scores": [85, 92, 78]
    })";
	doc.Parse(json);
    // 解析 JSON 字符串
    if (doc.HasParseError()) {
        cout << "Error parsing JSON!" << endl;
        return 1;
    }
    // 3. 访问 JSON 数据
    // 获取成员 "name"
    if (doc.HasMember("name") && doc["name"].IsString()) {
        cout << "name: " << doc["name"].GetString() << endl;
    }

    // 获取成员 "age"
    if (doc.HasMember("age") && doc["age"].IsInt()) {
        cout << "Age: " << doc["age"].GetInt() << endl;
    }

    // 获取成员 "isStudent"
    if (doc.HasMember("isStudent") && doc["isStudent"].IsBool()) {
        cout << "Is Student: " << (doc["isStudent"].GetBool() ? "Yes" : "No") << endl;
    }

    // 获取数组 "scores"
    if (doc.HasMember("scores") && doc["scores"].IsArray()) {
        const Value& scores = doc["scores"];
        for (SizeType i = 0; i < scores.Size(); i++) {
            cout << "Score " << i+1 << ": " << scores[i].GetInt() << endl;
        }
    }

    // 4. 修改 JSON 数据
    doc["age"] = 31;  // 修改 age 的值

    // 5. 添加新的成员
    Value address(kObjectType);
    address.AddMember("street", "123 Main St", doc.GetAllocator());
    address.AddMember("city", "New York", doc.GetAllocator());
    doc.AddMember("address", address, doc.GetAllocator());

    // 6. 输出修改后的 JSON
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    // 输出
    cout << buffer.GetString() << endl;

    return 0;
}
