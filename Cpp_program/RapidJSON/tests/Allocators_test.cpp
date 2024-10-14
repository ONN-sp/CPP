#include <iostream>
#include <vector>
#include "../include/allocators.h"

int main() {
    // 创建一个 StdAllocator<int> 分配器，使用默认的 CrtAllocator
    RAPIDJSON::StdAllocator<int> allocator;
    // 定义一个使用 StdAllocator 的 vector
    std::vector<int, RAPIDJSON::StdAllocator<int>> vec(allocator);
    // 向 vector 添加元素
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    // 打印 vector 元素
    for (const auto& elem : vec) {
        std::cout << elem << " ";  // 输出：1 2 3
    }
    std::cout << std::endl;
    return 0;
}
