#ifndef NONCOPYANLE_H
#define NONCOPYANLE_H

namespace tiny_muduo{
    class NonCopyAble{
        protected:
            // 构造函数和析构函数设为protected以防止直接实例化
            NonCopyAble() = default;
            ~NonCopyAble() = default;
        private:
            // 禁用拷贝构造函数和拷贝赋值运算符
            NonCopyAble(const NonCopyAble&) = delete;
            NonCopyAble& operator=(const NonCopyAble&) = delete;
    };
}
#endif