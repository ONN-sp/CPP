#include <iostream>
#include <vector>
#include "../include/allocators.h"

using namespace RAPIDJSON;

int main() {
    // 创建一个 StdAllocator<int> 分配器，使用默认的 CrtAllocator
    StdAllocator<int, CrtAllocator> allocator;
    // 定义一个使用 StdAllocator 的 vector
    std::vector<int, StdAllocator<int, CrtAllocator>> vec(allocator);
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
	
	// // 创建一个 StdAllocator<int> 分配器，使用默认的 CrtAllocator
	// MemoryPoolAllocator<CrtAllocator> poolAllocator;
    // StdAllocator<int, MemoryPoolAllocator<CrtAllocator>> allocator;
    // // 定义一个使用 StdAllocator 的 vector
    // std::vector<int, StdAllocator<int, MemoryPoolAllocator<CrtAllocator>>> vec(allocator);
    // // 向 vector 添加元素
    // vec.push_back(1);
    // vec.push_back(2);
    // vec.push_back(3);
    // // 打印 vector 元素
    // for (const auto& elem : vec) {
        // std::cout << elem << " ";  // 输出：1 2 3
    // }
    // std::cout << std::endl;
    // return 0
	
	// 定义内存池大小（根据需要调整）
    // const size_t kPoolSize = 1024; // 1 KB
    // // 创建内存池分配器
    // MemoryPoolAllocator<CrtAllocator> allocator(kPoolSize);
    // // 使用 MemoryPoolAllocator 分配内存
    // void* array = allocator.Malloc(1); // 分配一个包含 5 个整数的数组
    // if (array == nullptr) {
        // std::cerr << "Memory allocation failed!" << std::endl;
        // return 1;
    // }
    // std::cout << allocator.Capacity() << std::endl;
    // // 使用 MemoryPoolAllocator 释放内存
    // allocator.Free(array); // 释放数组内存
    // return 0;
}

