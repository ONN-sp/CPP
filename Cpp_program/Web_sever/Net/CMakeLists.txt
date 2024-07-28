cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(NetLib)
# 设置默认的构建类型为 "Release")，如果用户未指定
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

SET(CXX_FLAGS
  -g # 类似g++ -g
  -Wall # 启用大部分编译器警告
  -Wextra # 启用更多的警告
  -Werror # 将所有警告视为错误
  -Wconversion # 警告隐式类型转换可能导致的数据丢失或行为异常
  -Wshadow # 警告变量名隐藏   即内部作用域变量名可能会遮住外层作用域的该变量名
)
# 定义源文件列表
# 这里列出了所有要包含到静态库中的 .cpp 文件
set(NET_SOURCES
	Poller/DefaultPoller.cpp
    Poller/Epoller.cpp
    Poller/Poller.cpp
	Timer/Timer.cpp
	Timer/TimerQueue.cpp
	Util/Channel.cpp
	Util/EventLoop.cpp
	Util/Acceptor.cpp
	Util/Buffer.cpp
	Util/EventLoopThread.cpp
	Util/EventLoopThreadPool.cpp
	Util/Socket.cpp
	Util/Tcpconnection.cpp
	Util/Tcpserver.cpp
	Http/HttpContent.cpp
	Http/HttpRequest.cpp
	Http/HttpResponse.cpp
	Http/HttpServer.cpp
)
# 添加当前目录到搜索路径中
set(HEADERS
	Poll/Epoller.h
	Poll/Poller.h
	Util/Acceptor.h
	Util/Buffer.h
	Util/Channel.h
	Util/EventLoop.h
	Util/EventLoopThread.h
	Util/EventLoopThreadPool.h
	Util/Socket.h
	Util/Tcpconnection.h
	Util/Tcpserver.h
	Timer/Timer.h
	Timer/TimerQueue.h
	Http/HttpContent.h
	Http/HttpRequest.h
	Http/HttpResponse.h
	Http/HttpServer.h
	Http/HttpRequestParseState.h
	Http/HttpResponseFile.h
)
# 添加静态库目标 webserver_poller
add_library(webserver_net ${NET_SOURCES})
target_link_libraries(webserver_net webserver_base)

# 标准库不需要链接

# 安装目标文件（静态库）
# 将构建的静态库安装到 /usr/local/include/lib 目录
install(TARGETS webserver_net DESTINATION lib)

# 安装头文件
# 获取当前目录下所有以 .h 结尾的头文件并安装到 /usr/local/include/webserver 目录
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/webserver/net)