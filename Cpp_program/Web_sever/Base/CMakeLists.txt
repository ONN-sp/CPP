cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(BaseLib)

# 设置默认的构建类型为 Release，如果用户未指定
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
set(BASE_SOURCES
    Timestamp.cpp
	Thread.cpp
	./Logging/Logging.cpp
	./Logging/Logfile.cpp
	./Logging/Asynclogging.cpp
	./Logging/Logstream.cpp
)

# 添加当前目录到搜索路径中
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Logging)

# 添加静态库目标 webserver_base
add_library(webserver_base ${BASE_SOURCES})

# 标准库不需要链接

# 安装目标文件（静态库）
# 将构建的静态库安装到 /usr/local/lib 目录
install(TARGETS webserver_base DESTINATION lib)

# 安装头文件
# 获取当前目录下所有以 .h 结尾的头文件并安装到 /usr/local/include/webserver/Base 目录
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/webserver/Base)