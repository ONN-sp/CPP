#include "HttpRequest.h"
#include <algorithm>

using namespace tiny_muduo;

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}
// 解析请求方法
bool HttpRequest::ParseRequestMethod(const char* start, const char* end){// 从给定范围的字符串中找Http方法
    std::string request_method(start, end);// 复制字符串到request_method
    bool has_method = true;
    if(request_method == "GET")
        method_ = Method::kGet;
    else if (request_method == "POST") 
        method_ = Method::kPost;
    else if (request_method == "PUT") 
        method_ = Method::kPut;
    else if (request_method == "DELETE") 
        method_ = Method::kDelete;
    else if (request_method == "TRACE") 
        method_ = Method::kTrace;
    else if (request_method == "OPTIONS") 
        method_ = Method::kOptions;
    else if (request_method == "CONNECT") 
        method_ = Method::kConnect;
    else if (request_method == "PATCH")
        method_ = Method::kPatch;
    else
        has_method = false;
    return has_method;// 没有解析到给定的几种Http方法就会返回false
}
// 解析请求行
bool HttpRequest::ParseRequestLine(const char* start, const char* end){// 从指定范围的Http报文的一行进行解析
    const char* space = nullptr;
    space = std::find(start, end, ' ');// 查找第一个空格位置,标记方法和路径的分割处
    if(space == end)
        return false;// 没有找到空格说明请求行的格式有错
    if(!ParseRequestMethod(start, space))// 请求行中  初始start--第一个空格  就是Http报文的方法
        return false;
    start = space + 1;// 开始解析后半部分
    space = std::find(start, end, ' ');// 查找下一个空格位置,标记路径和Http版本的分割处
    if(space == end)
        return false;
    // 查找路径中的查询字符串
    const char* query_ptr = std::find(start, end, '?');
    if(query_ptr!=end){// 找到查询字符串,就分别赋值路径和查询字符串
        path_.assign(start, query_ptr);
        query_.assign(query_ptr+1, space);
    }
    else// 不存在查询字符串,就只赋值路径
        path_.assign(start, space);
    start = space + 1;// 移到HTTP版本处
    bool parsehttp_version = (start + 8 == end)&&(std::equal(start, end-1, "HTTP/1.") || std::equal(start, end-1, "HTTP/2."));
    if(!parsehttp_version){// 不是HTTP正确版本格式,返回false
        version_ = Version::kUnknown;
        return false;
    }
    // 根据最后一个字符&&倒数第二个字符设置HTTP版本
    if((*(end-3)=='2')&&(*(end-1)=='0'))// !!! -3,不是-2
        version_ = Version::kHttp20;
    else if((*(end-3)=='1')&&(*(end-1)=='0'))
        version_ = Version::kHttp10;
    else
        version_ = Version::kHttp11;
    return true;
}
// 解析请求主体
bool HttpRequest::ParseBody(const char* start, const char* end){
    (void)start;
    (void)end;
    return true;
}
// 获取请求头集合
bool HttpRequest::ParseHeaders(const char* start, const char* colon, const char* end){// colon:冒号的位置
    const char* validstart = colon + 1;// 将有效值的起始位置设为冒号后一个字符的位置
    while(*validstart == ' ')// 跳过有效值前的空格
        validstart++;
    // 从 start 到 colon 创建一个 string 并移动到 map 的 key 中
    // 从 validstart 到 end 创建一个 string 并移动到 map 的 value 中
    headers_[std::move(std::string(start, colon))] = std::move(std::string(validstart, end));
    return true;
}
// 交换HttpRequest对象
void HttpRequest::Swap(HttpRequest& req){
    method_ = req.method_;
    version_ = req.version_;
    path_.swap(req.path_);
    query_.swap(req.query_);
    headers_.swap(req.headers_);
}
// 获取指定请求头的值  在headers_通过给定的请求头(key)找对应的value
std::string HttpRequest::GetHeader(const std::string& header) const {
    std::string ret;
    auto iter = headers_.find(header);
    return iter == headers_.end() ? ret : iter->second;
}
