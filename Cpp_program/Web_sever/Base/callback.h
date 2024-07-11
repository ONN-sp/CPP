// 下面的回调都是上层回调  channel中的回调不在这定义
#ifndef CALLBACK_H
#define CALLBACK_H

#include <memory>
#include <functional>

namespace tiny_muduo{
    class TcpConnection;//前向声明
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    class Buffer;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
}
#endif