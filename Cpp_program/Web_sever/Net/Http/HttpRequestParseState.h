#ifndef HTTPREQUESTPARSESTATE_H
#define HTTPREQUESTPARSESTATE_H

namespace tiny_muduo{// 定义HTTP的请求的解析状态
    enum class HttpRequestParseState{
        kParseRequestLine,// 解析请求行 (例如：GET /index.html HTTP/1.1)
        kParseHeaders,// 解析头部字段 (例如：Host: www.example.com)
        kParseBody,// 解析请求内容主体 (例如：POST 请求中的body数据)
        kParseGotCompleteRequest,// 请求消息已经完整被解析了(本项目中请求行和头部字段被处理完了就是完整被解析了)
        kParseErron,// 解析过程出错
    };
}
#endif