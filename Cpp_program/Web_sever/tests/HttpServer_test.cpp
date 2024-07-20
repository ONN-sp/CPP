#include <stdio.h>
#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <climits>

#include "../Net/Util/EventLoop.h"
#include "../Base/Address.h"
#include "../Net/Http/HttpContent.h"
#include "../Net/Http/HttpServer.h"
#include "../Net/Http/HttpRequest.h"
#include "../Net/Http/HttpResponse.h"
#include "../Net/Http/HttpResponseFile.h"
#include "../Base/Logging/Logging.h"

using namespace tiny_muduo;

void HttpResponseCallback(const HttpRequest& request, HttpResponse& response) {
  if (request.method() != Method::kGet) {
    response.SetStatusCode(HttpStatusCode::k400BadRequest);
    response.SetStatusMessage("Bad Request");
    response.SetCloseConnection(true);
    return;
  }    
  
  {
    const std::string& path = request.path(); 
    if (path == "/") {
      response.SetStatusCode(HttpStatusCode::k200OK);
      response.SetBodyType("text/html");
      response.SetBody(DrewJun_website);
    } else if (path == "/hello") {
      response.SetStatusCode(HttpStatusCode::k200OK);
      response.SetBodyType("text/html");
      response.SetBody("Hello, world!\n");
    } else if (path == "/favicon.ico" || path == "/favicon") {
      response.SetStatusCode(HttpStatusCode::k200OK);
      response.SetBodyType("image/png");
      response.SetBody(std::string(favicon, sizeof(favicon))); 
    } else {
      response.SetStatusCode(HttpStatusCode::k404NotFound);
      response.SetStatusMessage("Not Found");
      response.SetCloseConnection(true);
      return;
    }
  }
}

int main() {
  EventLoop loop;
  Address listen_address("127.0.0.1", "9999");// 利用port作为构造Address的参数
  HttpServer server(&loop, listen_address);
  server.SetThreadNums(4);
  server.SetHttpResponseCallback(HttpResponseCallback);
  server.Start();
  loop.loop();
  return 0;
}