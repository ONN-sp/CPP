# 学习笔记
1. <mark>查询字符串是指在`URL`中用于传递参数的部分,通常出现在`?`后面,用于向服务器传递参数或数据,它由一系列键值对组成,键值对之间以及键和值之间用`=`分割,如:`?http://example.com/page?param1=value1&param2=value2`</mark>
2. 当客户端发送的请求不需要传递任何参数给服务器,则此时的`url`中没有查询字符串
## getsockname
1. 此函数用于获取套接字的本地地址信息,即本地主机和端口号
2. `getsockname`第二个参数期望一个指向`sockaddr`结构体的指针,而<mark>`reinterpret_cast`是`C++`中的一个类型转换操作符,用于在不同类型之间进行强制类型转换,它的作用是将一个指针或引用转换为一个完全不同的类型,即使这两种类型没有任何继承关系</mark>:
   ```C++
   reinterpret_cast<new_type>(expression)
   //expression是要进行转换的表达式,可以是一个指针、引用、或者其它任意表达式
   ```
## isspace

# strcasecmp()  C函数
1. `strcasecmp`不区分大小写;而`std::strcmp`要区分大小写

##  stat
1. `C++`中,可以使用`stat`函数来获取文件的状态信息,包括文件的大小、权限、修改时间等,这个函数是通过`<sys/stat.h>`头文件调用:
   ```C++
   struct stat st;
   st.st_size;
   st.st_mode;
   &st.st_mtime;
   ```
2. `st.st_mode`中的`st_mode`是`struct stat`结构体中的一个成员,用于表示文件的类型和访问权限,可以使用宏来解析`st_mode`成员的值,以确定文件的类型和权限:
   ```C++
   st.st_mode & S_ISREG;//判断是否是普通文件
   st.st_mode & S_IXUSR;//检查文件的指向权限是否被设置为用户可执行
   st.st_mode & S_IXGRP;//检查文件的指向权限是否被设置为群组可执行
   st.st_mode & S_IXOTH;//检查文件的指向权限是否被设置为其它用户可执行
   ```


