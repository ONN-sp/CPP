#ifndef CALLBACK_H
#define CALLBACK_H

#include <memory>
#include <functional>

namespace tiny_muduo{
    class TcpConnection;//前向声明
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    class Buffer;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
    using ReadCallback = std::function<void()>;
    using WriteCallback = std::function<void()>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    using ErrorCallback = std::function<void()>;
}
#endif