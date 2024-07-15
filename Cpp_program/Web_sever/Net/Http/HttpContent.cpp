#include "HttpContent.h"
#include <algorithm>

using namespace tiny_muduo;

HttpContent::HttpContent()
    : parse_state_(HttpRequestParseState::kParseRequestLine)// 初始化为解析行状态  Http报文一进来首先解析行
    {}

HttpContent::~HttpContent(){}

bool HttpContent::ParseContent(Buffer* buffer){
    bool linemore = true;// 是否继续解析行
    bool parseok = true;// 解析是否成功
    const char* CRLF = nullptr;// 指向行结束符(换行符)
    while(linemore){
        CRLF = nullptr;
        if(parse_state_ == HttpRequestParseState::kParseRequestLine){// 解析行状态(表示的是进入解析行状态)
            CRLF = buffer->FindCRLF();// 查找请求行的行结束符
            if(CRLF){// 如果找到就解析请求行 调用kParseRequestLine
                parseok = request_.ParseRequestLine(buffer->Peek(), CRLF);
                if(parseok)
                    parse_state_ = HttpRequestParseState::kParseHeaders;// 请求行解析成功了,就进入解析头部字段状态
                else
                    linemore = false;// 请求行解析失败,停止解析
            }
            else// 没有找到换行符,直接停止解析
                linemore = true;       
        }
        else if(parse_state_ == HttpRequestParseState::kParseHeaders){// 进入解析头部字段状态
            CRLF = buffer->FindCRLF();
            if(CRLF){
                const char* colon = std::find((const char*)buffer->Peek(), CRLF, ':');// 查找用于分割头部字段的key和value的冒号的位置
                if(colon == CRLF){// 没找到冒号
                    parse_state_ = HttpRequestParseState::kParseGotCompleteRequest;
                    linemore = false;
                }
                else{
                    parseok = request_.ParseHeaders(buffer->Peek(), colon, CRLF);// 解析头部字段
                    if(!parseok)// 解析头部字段失败
                        linemore = false;
                }
            }
            else
                linemore = false;// 没有找到换行符,直接停止解析
        }
        else if(parse_state_ == HttpRequestParseState::kParseGotCompleteRequest)// 进入完整解析状态, 即解析完整了
            linemore = false;
        else if(parse_state_ == HttpRequestParseState::kParseBody){
            // 未实现
        }
        if(CRLF)
            buffer->RetrieveUntilIndex(CRLF + 2);// 解析完毕  移动buffer的readIndex指针
    }
    return parseok;
}

void HttpContent::ResetContentState(){
    HttpRequest tmp;
    request_.Swap(tmp);
    parse_state_ = HttpRequestParseState::kParseRequestLine;// 重置解析状态
}