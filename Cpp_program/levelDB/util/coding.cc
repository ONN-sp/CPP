#include "util/coding.h"

namespace leveldb {
    /**
     * @brief 封装EncodeFixed32，将结果追加到字符串
     * 
     * @param dst 
     * @param value 
     */
    void PutFixed32(std::string* dst, uint32_t value) {
        char buf[sizeof(value)];
        EncodeFixed32(buf, value);
        dst->append(buf, sizeof(buf));
    }
    /**
     * @brief 封装EncodeFixed64，将结果追加到字符串
     * 
     * @param dst 
     * @param value 
     */
    void PutFixed64(std::string* dst, uint64_t value) {
        char buf[sizeof(value)];
        EncodeFixed64(buf, value);
        dst->append(buf, sizeof(buf));
    }
    /**
     * @brief 32位数据变长编码
     * 
     * @param dst 
     * @param v 
     * @return char* 
     */
    char* EncodeVarint32(char* dst, uint32_t v) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(dst);
        static const int B = 128;
        if (v < (1 << 7))
            *(ptr++) = v;
        else if (v < (1 << 14)) {
            *(ptr++) = v | B;
            *(ptr++) = v >> 7;
        } 
        else if (v < (1 << 21)) {
            *(ptr++) = v | B;
            *(ptr++) = (v >> 7) | B;
            *(ptr++) = v >> 14;
        } 
        else if (v < (1 << 28)) {
            *(ptr++) = v | B;
            *(ptr++) = (v >> 7) | B;
            *(ptr++) = (v >> 14) | B;
            *(ptr++) = v >> 21;
        } 
        else {
            *(ptr++) = v | B;
            *(ptr++) = (v >> 7) | B;
            *(ptr++) = (v >> 14) | B;
            *(ptr++) = (v >> 21) | B;
            *(ptr++) = v >> 28;
        }
        return reinterpret_cast<char*>(ptr);
    }
    /**
     * @brief 封装EncodeVarint32，将结果追加到字符串
     * 
     * @param dst 
     * @param v 
     */
    void PutVarint32(std::string* dst, uint32_t v) {
        char buf[5];
        char* ptr = EncodeVarint32(buf, v);
        dst->append(buf, ptr - buf);
    }
    /**
     * @brief 64位数据变长编码
     * 
     * @param dst 
     * @param v 
     * @return char* 
     */
    char* EncodeVarint64(char* dst, uint64_t v) {
        static const int B = 128;
        uint8_t* ptr = reinterpret_cast<uint8_t*>(dst);
        while (v >= B) {
            *(ptr++) = v | B;
            v >>= 7;
        }
        *(ptr++) = static_cast<uint8_t>(v);
        return reinterpret_cast<char*>(ptr);
    }
    /**
     * @brief 封装EncodeVarint64
     * 
     * @param dst 
     * @param v 
     */
    void PutVarint64(std::string* dst, uint64_t v) {
        char buf[10];
        char* ptr = EncodeVarint64(buf, v);
        dst->append(buf, ptr - buf);
    }
    /**
     * @brief 写入带长度前缀的Slice数据,即如MemTable Key的前面32位长度数据(键长度)
     * 
     * @param dst 
     * @param value 
     */
    void PutLengthPrefixedSlice(std::string* dst, const Slice& value) {
        PutVarint32(dst, value.size());
        dst->append(value.data(), value.size());
    }
    /**
     * @brief 计算Varint编码所需字节数
     * 
     * @param v 
     * @return int 
     */
    int VarintLength(uint64_t v) {
        int len = 1;
        while (v >= 128) {
            v >>= 7;
            len++;
        }
        return len;
    }
    /**
     * @brief 32位数据变长解码
     * 
     * @param p 
     * @param limit 
     * @param value 
     * @return const char* 
     */
    const char* GetVarint32PtrFallback(const char* p, const char* limit,
                                    uint32_t* value) {
        uint32_t result = 0;
        for (uint32_t shift = 0; shift <= 28 && p < limit; shift += 7) {
            uint32_t byte = *(reinterpret_cast<const uint8_t*>(p));
            p++;
            if (byte & 128)
                result |= ((byte & 127) << shift);
            else {
                result |= (byte << shift);
                *value = result;
                return reinterpret_cast<const char*>(p);
            }
        }
        return nullptr;
    }
    /**
     * @brief 封装GetVarint32Ptr，处理Slice输入
     * 
     * @param input 
     * @param value 
     * @return true 
     * @return false 
     */
    bool GetVarint32(Slice* input, uint32_t* value) {
        const char* p = input->data();
        const char* limit = p + input->size();
        const char* q = GetVarint32Ptr(p, limit, value);
        if (q == nullptr)
            return false;
        else {
            *input = Slice(q, limit - q);
            return true;
        }
    }
    /**
     * @brief 封装GetVarint64Ptr，处理Slice输入
     * 
     * @param p 
     * @param limit 
     * @param value 
     * @return const char* 
     */
    const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* value) {
        uint64_t result = 0;
        for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7) {
            uint64_t byte = *(reinterpret_cast<const uint8_t*>(p));
            p++;
            if (byte & 128)
                result |= ((byte & 127) << shift);
            else {
                result |= (byte << shift);
                *value = result;
                return reinterpret_cast<const char*>(p);
            }
        }
        return nullptr;
    }
    /**
     * @brief 64位变长解码
     * 
     * @param input 
     * @param value 
     * @return true 
     * @return false 
     */
    bool GetVarint64(Slice* input, uint64_t* value) {
        const char* p = input->data();
        const char* limit = p + input->size();
        const char* q = GetVarint64Ptr(p, limit, value);
        if (q == nullptr)
            return false;
        else {
            *input = Slice(q, limit - q);
            return true;
        }
    }
    /**
     * @brief 解码带长度前缀的Slice数据,前8位是长度数据
     * 
     * @param input 
     * @param result 
     * @return true 
     * @return false 
     */
    bool GetLengthPrefixedSlice(Slice* input, Slice* result) {
        uint32_t len;
        if (GetVarint32(input, &len) && input->size() >= len) {
            *result = Slice(input->data(), len);
            input->remove_prefix(len);
            return true;
        } 
        else 
            return false;
    }
}