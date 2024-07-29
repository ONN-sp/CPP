
cmake_minimum_required(VERSION 3.0)# 此项目要求的最低 CMake 版本为 3.0

project(Test CXX)
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
# add_executable(Timestamp_test Timestamp_test.cpp)# 指定生成目标
# target_link_libraries(Timestamp_test webserver_base)

# add_executable(TimerQueue_test TimerQueue_test.cpp)# 指定生成目标
# target_link_libraries(TimerQueue_test webserver_net webserver_base)

# add_executable(Logfile_stream_test Logfile_stream_test.cpp)# 指定生成目标
# target_link_libraries(Logfile_stream_test webserver_base)

# add_executable(Asynclogging_test Asynclogging_test.cpp)# 指定生成目标
# target_link_libraries(Asynclogging_test webserver_base)

# add_executable(TcpServer_test TcpServer_test.cpp)# 指定生成目标
# target_link_libraries(TcpServer_test webserver_base webserver_net) 

add_executable(HttpServer_test HttpServer_test.cpp)# 指定生成目标
target_link_libraries(HttpServer_test webserver_net webserver_base)# 链接顺序千万不要反过来