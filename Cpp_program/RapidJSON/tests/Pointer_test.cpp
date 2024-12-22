#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

using namespace RAPIDJSON;
using namespace std;

int main() {
    // 示例 JSON 数据
    const char* json = R"({
        "article": {
            "id": "article/123",
            "details": {
                "id": "article/123/details"
            }
        },
        "image": {
            "id": "image/456"
        }
    })";

    // 创建一个 RAPIDJSON 文档对象并解析 JSON 字符串
    Document doc;
    doc.Parse(json);

    Pointer::UriType rootUri("http://example.com/");

    // 示例：获取"/article/details"路径的 URI
    GenericPointer<Value> pointer("/article/details");
    Pointer::UriType finalUri = pointer.GetUri(doc, rootUri);

    // 打印解析后的 URI
    std::cout << "Final URI: " << finalUri.GetString() << std::endl;

    // 进一步测试路径解析
    Pointer::UriType finalUri2 = pointer.GetUri(doc, rootUri);
    std::cout << "Final URI (2): " << finalUri2.GetString() << std::endl;

    return 0;
}
