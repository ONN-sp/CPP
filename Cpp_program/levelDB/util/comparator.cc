#include "leveldb/comparator.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <type_traits>
#include "leveldb/slice.h"
#include "util/logging.h"
#include "util/no_destructor.h"

namespace leveldb {
    Comparator::~Comparator() = default;
    namespace {
        class BytewiseComparatorImpl : public Comparator {
            public:
            BytewiseComparatorImpl() = default;
            // 返回比较器名称标识
            const char* Name() const override { return "leveldb.BytewiseComparator"; }
            // 比较两个 Slice 数据
            int Compare(const Slice& a, const Slice& b) const override {
                return a.compare(b);
            }
            // 寻找 start 和 limit 之间的最短分隔符，并修改 start
            void FindShortestSeparator(std::string* start, const Slice& limit) const override {
                size_t min_length = std::min(start->size(), limit.size());
                size_t diff_index = 0;
                while ((diff_index < min_length) && ((*start)[diff_index] == limit[diff_index]))
                    diff_index++;
                if (diff_index >= min_length)// 说明*start是limit的前缀，或者反之，此时不作修改，直接返回 
                {} 
                else {
                    uint8_t diff_byte = static_cast<uint8_t>((*start)[diff_index]);
                    if (diff_byte < static_cast<uint8_t>(0xff) && diff_byte + 1 < static_cast<uint8_t>(limit[diff_index])) {// 检查是否可以递增该字节且不越过 limit 的对应字节
                        (*start)[diff_index]++;// 递增该字节，例如：start="abc1", limit="abc3" -> "abc2"
                        start->resize(diff_index + 1);// 截断后续字节，缩短长度
                        assert(Compare(*start, limit) < 0);// 验证新 start 必须小于 limit（仅在调试模式生效）
                    }
                }
            }
            // 找到比当前键大的最短键，通过递增第一个非 0xFF 字节并截断   即生成最小后继键
            void FindShortSuccessor(std::string* key) const override {
                // 找到第一个可以++的字符，执行++后，截断字符串；
                // 如果找不到说明*key的字符都是0xff啊，那就不作修改，直接返回
                size_t n = key->size();
                for (size_t i = 0; i < n; i++) {
                    const uint8_t byte = (*key)[i];
                    if (byte != static_cast<uint8_t>(0xff)) {
                        (*key)[i] = byte + 1;
                        key->resize(i + 1);
                        return;
                    }
                }
            }
        };
    }
    // 获取字节序比较器单例实例
    const Comparator* BytewiseComparator() {
        static NoDestructor<BytewiseComparatorImpl> singleton;
        return singleton.get();
    }
}