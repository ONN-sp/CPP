#ifndef HTTPCONTEN_H
#define HTTPCONTEN_H

#include "../../Base/NonCopyAble.h"
#include "../Util/Buffer.h"
#include "HttpRequest.h"
#include "HttpRequestParseState.h"

namespace tiny_muduo{
    class HttpContent{// 解析HTTP请求的内容
        public:
            HttpContent();
            ~HttpContent();
            // 解析缓冲区内容(请求行+头部字段),返回是否解析成功
            bool ParseContent(Buffer*);
            // 获取是否已经完成请求的解析的一个状态  即判断是否已经完成解析
            bool GetCompleteRequest(){ return parse_state_ == HttpRequestParseState::kParseGotCompleteRequest;}
            // 获取HttpRequest对象的常量引用
            const HttpRequest& request() const { return request_;}
            // 重置内容的解析状态
            void ResetContentState();
        private:
            HttpRequest request_;// HTTP请求对象
            HttpRequestParseState parse_state_;// HTTP当前的请求解析状态
    };
}
#endif