#include "logstream.h"

using namespace tiny_muduo;

//GeneralTemplate类的定义
GeneralTemplate::GeneralTemplate() : data_(nullptr), len_(0){}

GeneralTemplate::GeneralTemplate(const char* data, int len) : data_(data), len_(len){}

template <int SIZE>
void FixedBuffer<SIZE>::CookieStart() {}

template <int SIZE>
void FixedBuffer<SIZE>::CookieEnd() {}

// 显式实例化模板类 FixedBuffer，实例化大小为 kLargeSize 的实例
template class FixedBuffer<kLargeSize>;

// 显式实例化模板类 FixedBuffer，实例化大小为 kSmallSize 的实例
template class FixedBuffer<kSmallSize>;
