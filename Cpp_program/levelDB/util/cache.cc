#include "cache.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "../../port/port.h"
#include "../../port/thread_annotations.h"
#include "../../util/hash.h"
#include "../../util/mutexlock.h"

namespace leveldb {
    Cache::~Cache() {}
    namespace {
        /**
         * @brief LRU缓存节点结构体，代表缓存中的一个节点,既用在哈希表中(键节点),又用在后续双向链表中(value节点)
         * 同一个 LRUHandle 节点会被哈希表和双向链表共享,所以LRUHandle结构中既有key又有value
         */
        struct LRUHandle {
            void* value;// 节点中保存的值
            void (*deleter)(const Slice&, void* value);// 节点的销毁方法
            LRUHandle* next_hash;// 如果产生哈希冲突,则用一个单向链表串联冲突的节点
            LRUHandle* next;// LRU双向链表的后一个节点
            LRUHandle* prev;// LRU双向链表的前一个节点
            size_t charge;// 当前节点占用的容量
            size_t key_length;// 节点中键的长度
            bool in_cache;// 该节点是否在缓存中,如果不再,则可以调用节点的销毁方法
            uint32_t refs;// 该节点的引用计数 
            uint32_t hash;// 该节点键的哈希值
            char key_data[1];// 该节点的占位符,键的实际长度保存在key_length中
            Slice key() const {
                assert(next != this);
                return Slice(key_data, key_length);
            }
        };
        /**
         * @brief 哈希表类，处理LRU节点的快速查找（链地址法解决冲突）
         * 
         */
        class HandleTable {
            public:
            HandleTable() : length_(0), elems_(0), list_(nullptr) { Resize(); }
            ~HandleTable() { delete[] list_; }
            // 查找键对应的节点指针的指针（便于插入/删除操作）
            LRUHandle* Lookup(const Slice& key, uint32_t hash) {
                return *FindPointer(key, hash);
            }
            // 插入节点，返回被替换的旧节点（若存在）
            LRUHandle* Insert(LRUHandle* h) {
                LRUHandle** ptr = FindPointer(h->key(), h->hash);
                LRUHandle* old = *ptr;
                h->next_hash = (old == nullptr ? nullptr : old->next_hash);
                *ptr = h;// 将新节点插入链表头部,最新
                if (old == nullptr) {
                    ++elems_;
                    if (elems_ > length_)// 哈希表扩容：当元素数超过表长时扩容为2倍
                        Resize();
                }
                return old;
            }
            // 删除指定键的节点，返回被删除的节点指针
            LRUHandle* Remove(const Slice& key, uint32_t hash) {
                LRUHandle** ptr = FindPointer(key, hash);
                LRUHandle* result = *ptr;
                if (result != nullptr) {
                    *ptr = result->next_hash;// 从链表中摘除
                    --elems_;
                }
                return result;
            }
            private:
                uint32_t length_;// 哈希表桶的数量（2的幂）
                uint32_t elems_;// 当前存储的元素总数
                LRUHandle** list_;// 哈希桶数组（每个元素是链表头指针）
                LRUHandle** FindPointer(const Slice& key, uint32_t hash) {// 查找键对应的节点指针的指针
                    LRUHandle** ptr = &list_[hash & (length_ - 1)];// 计算桶索引：hash & (length_ - 1) 替代取模运算
                    while (*ptr != nullptr && ((*ptr)->hash != hash || key != (*ptr)->key())) // 遍历冲突链，直到找到匹配的节点或链表末尾
                        ptr = &(*ptr)->next_hash;
                    return ptr;
                }
                 // 调整哈希表大小（翻倍）
                void Resize() {
                    uint32_t new_length = 4;// 初始大小为4
                    while (new_length < elems_)
                        new_length *= 2;
                    LRUHandle** new_list = new LRUHandle*[new_length];
                    memset(new_list, 0, sizeof(new_list[0]) * new_length);
                    uint32_t count = 0;
                    // 重新哈希所有现有节点到新表
                    for (uint32_t i = 0; i < length_; i++) {
                        LRUHandle* h = list_[i];
                        while (h != nullptr) {
                            LRUHandle* next = h->next_hash;
                            uint32_t hash = h->hash;
                            LRUHandle** ptr = &new_list[hash & (new_length - 1)];
                            h->next_hash = *ptr;// 插入到新桶的链表头部
                            *ptr = h;
                            h = next;
                            count++;
                        }
                    }
                    assert(elems_ == count);
                    delete[] list_;// 释放旧表内存
                    list_ = new_list;
                    length_ = new_length;
                }
        };
        /**
         * @brief LRU缓存实现类
         * 
         */
        class LRUCache {
            public:
                LRUCache();
                ~LRUCache();
                void SetCapacity(size_t capacity) { capacity_ = capacity; }// 设置缓存容量上限
                // 插入键值对到缓存，返回Handle（若内存超限触发LRU淘汰）
                Cache::Handle* Insert(const Slice& key, uint32_t hash, void* value, size_t charge, void (*deleter)(const Slice& key, void* value));
                Cache::Handle* Lookup(const Slice& key, uint32_t hash);// 查找键，增加引用计数并返回Handle
                void Release(Cache::Handle* handle);// 释放Handle的引用，可能将节点移回LRU链表
                void Erase(const Slice& key, uint32_t hash);// 显式删除指定键的缓存项
                void Prune();// 清理所有可回收的LRU节点（引用计数为1）
                size_t TotalCharge() const {// 获取当前缓存总占用
                    MutexLock l(&mutex_);
                    return usage_;
                }
            private:
                void LRU_Remove(LRUHandle* e);// 从LRU链表中移除节点
                void LRU_Append(LRUHandle* list, LRUHandle* e);// 将节点追加到指定链表的末尾
                void Ref(LRUHandle* e);// 增加节点引用，若从LRU链移至使用链
                void Unref(LRUHandle* e);// 减少引用，引用为0时销毁，为1且in_cache则移回lru_链
                bool FinishErase(LRUHandle* e) EXCLUSIVE_LOCKS_REQUIRED(mutex_);// 完成节点删除操作，返回是否成功
                size_t capacity_;// 缓存容量上限
                mutable port::Mutex mutex_;
                size_t usage_ GUARDED_BY(mutex_);// 当前缓存总占用
                LRUHandle lru_ GUARDED_BY(mutex_);// LRU链表头（可淘汰的节点，refs=1）
                LRUHandle in_use_ GUARDED_BY(mutex_);// 使用中链表头（refs>=2）
                HandleTable table_ GUARDED_BY(mutex_);// 哈希表管理所有缓存节点
        };
        LRUCache::LRUCache() : capacity_(0), usage_(0) {// LRUCache构造函数：初始化双链表
            // 初始化lru_链表为自环（空链表）
            lru_.next = &lru_;
            lru_.prev = &lru_;
            // 初始化in_use_链表为自环
            in_use_.next = &in_use_;
            in_use_.prev = &in_use_;
        }
        LRUCache::~LRUCache() {
            assert(in_use_.next == &in_use_);
            for (LRUHandle* e = lru_.next; e != &lru_;) {// 遍历LRU链表，释放所有节点
                LRUHandle* next = e->next;
                assert(e->in_cache);
                e->in_cache = false;
                assert(e->refs == 1);
                Unref(e);
                e = next;
            }
        }
        // 增加节点引用计数，并维护链表位置
        void LRUCache::Ref(LRUHandle* e) {
            if (e->refs == 1 && e->in_cache) {// 当引用从1变为2时，从lru_链移至in_use_
                LRU_Remove(e);
                LRU_Append(&in_use_, e);
            }
            e->refs++;
        }
        // 减少引用计数，可能触发节点回收或销毁
        void LRUCache::Unref(LRUHandle* e) {
            assert(e->refs > 0);
            e->refs--;
            if (e->refs == 0) {// 无引用，立即销毁
                assert(!e->in_cache);
                (*e->deleter)(e->key(), e->value);// 调用用户回调清理数据
                free(e);
            } else if (e->in_cache && e->refs == 1) {// 引用降至1且仍在缓存，移回lru_链等待回收
                LRU_Remove(e);
                LRU_Append(&lru_, e);
            }
        }
        // 将节点e从所在链表中移除（修改前后节点的指针）
        void LRUCache::LRU_Remove(LRUHandle* e) {
            e->next->prev = e->prev;
            e->prev->next = e->next;
        }
        // 将节点e插入到list链表的末尾（头节点的前驱位置）
        void LRUCache::LRU_Append(LRUHandle* list, LRUHandle* e) {
            e->next = list;// e的后继指向头节点
            e->prev = list->prev;// e的前驱指向原末尾节点
            e->prev->next = e;// 原末尾节点的后继指向e
            e->next->prev = e;// 头节点的前驱指向e
        }
        // 查找缓存项：找到后增加引用计数并返回
        Cache::Handle* LRUCache::Lookup(const Slice& key, uint32_t hash) {
            MutexLock l(&mutex_);
            LRUHandle* e = table_.Lookup(key, hash);// 哈希表查找
            if (e != nullptr)
                Ref(e);// 命中，增加引用
            return reinterpret_cast<Cache::Handle*>(e);
        }
        // 释放缓存项引用：可能触发移至lru_链或销毁
        void LRUCache::Release(Cache::Handle* handle) {
            MutexLock l(&mutex_);
            Unref(reinterpret_cast<LRUHandle*>(handle));
        }
        // 插入新缓存项：分配节点，处理LRU淘汰
        Cache::Handle* LRUCache::Insert(const Slice& key, uint32_t hash, void* value, size_t charge, void (*deleter)(const Slice& key, void* value)) {
            MutexLock l(&mutex_);
            LRUHandle* e = reinterpret_cast<LRUHandle*>(malloc(sizeof(LRUHandle) - 1 + key.size()));// 分配LRUHandle内存（额外空间存储键数据）
            // 初始化节点字段
            e->value = value;
            e->deleter = deleter;
            e->charge = charge;
            e->key_length = key.size();
            e->hash = hash;
            e->in_cache = false;
            e->refs = 1;
            std::memcpy(e->key_data, key.data(), key.size());// 复制键数据
            if (capacity_ > 0) {// 缓存容量有效
                e->refs++;
                e->in_cache = true;
                LRU_Append(&in_use_, e);// 加入in_use_链表
                usage_ += charge;// 更新总内存占用
                FinishErase(table_.Insert(e));// 插入哈希表，替换旧节点并清理
            } 
            else // 零容量缓存，不实际缓存
                e->next = nullptr;
            while (usage_ > capacity_ && lru_.next != &lru_) {// 若当前内存超限，按LRU顺序淘汰
                LRUHandle* old = lru_.next;// lru_链首节点即最久未使用
                assert(old->refs == 1);
                bool erased = FinishErase(table_.Remove(old->key(), old->hash));
                if (!erased)// 此处应确保删除成功
                assert(erased);
            }
            return reinterpret_cast<Cache::Handle*>(e);
        }
        // 完成节点删除的后处理：更新链表和内存占用
        bool LRUCache::FinishErase(LRUHandle* e) {
            if (e != nullptr) {
                assert(e->in_cache);
                LRU_Remove(e);
                e->in_cache = false;
                usage_ -= e->charge;
                Unref(e);
            }
            return e != nullptr;// 返回是否实际执行了删除
        }
        // 删除指定键的缓存项
        void LRUCache::Erase(const Slice& key, uint32_t hash) {
            MutexLock l(&mutex_);
            FinishErase(table_.Remove(key, hash));// 哈希表删除并处理
        }
        // 清理所有可回收的LRU节点（引用计数为1）
        void LRUCache::Prune() {
            MutexLock l(&mutex_);
            while (lru_.next != &lru_) {// 遍历lru_链
                LRUHandle* e = lru_.next;
                assert(e->refs == 1);
                bool erased = FinishErase(table_.Remove(e->key(), e->hash));
                if (!erased) 
                    assert(erased);
            }
        }
        static const int kNumShardBits = 4; // 分片数对数（2^4=16）
        static const int kNumShards = 1 << kNumShardBits;// 分片数
        // 分片LRU缓存类，减少锁的粒度
        class ShardedLRUCache : public Cache {
            private:
                LRUCache shard_[kNumShards];// 分片数组
                port::Mutex id_mutex_;// 用于生成唯一ID的锁
                uint64_t last_id_;// 最后分配的ID
                static inline uint32_t HashSlice(const Slice& s) {// 计算键的哈希值（使用LevelDB内部哈希函数）
                    return Hash(s.data(), s.size(), 0);
                }
                static uint32_t Shard(uint32_t hash) { return hash >> (32 - kNumShardBits); }// 根据哈希值计算分片索引（取高4位）
            public:
                explicit ShardedLRUCache(size_t capacity) : last_id_(0) {// 构造函数：均分总容量到各分片
                    const size_t per_shard = (capacity + (kNumShards - 1)) / kNumShards;
                    for (int s = 0; s < kNumShards; s++)
                        shard_[s].SetCapacity(per_shard);
                }
                ~ShardedLRUCache() override {}
                // 插入到对应分片
                Handle* Insert(const Slice& key, void* value, size_t charge, void (*deleter)(const Slice& key, void* value)) override {
                    const uint32_t hash = HashSlice(key);
                    return shard_[Shard(hash)].Insert(key, hash, value, charge, deleter);
                }
                // 查找对应分片
                Handle* Lookup(const Slice& key) override {
                    const uint32_t hash = HashSlice(key);
                    return shard_[Shard(hash)].Lookup(key, hash);
                }
                // 释放对应分片的Handle
                void Release(Handle* handle) override {
                    LRUHandle* h = reinterpret_cast<LRUHandle*>(handle);
                    shard_[Shard(h->hash)].Release(handle);
                }
                // 删除对应分片的键
                void Erase(const Slice& key) override {
                    const uint32_t hash = HashSlice(key);
                    shard_[Shard(hash)].Erase(key, hash);
                }
                // 获取Handle中的值
                void* Value(Handle* handle) override {
                    return reinterpret_cast<LRUHandle*>(handle)->value;
                }
                // 生成唯一ID             
                uint64_t NewId() override {
                    MutexLock l(&id_mutex_);
                    return ++(last_id_);
                }
                // 清理所有分片
                void Prune() override {
                    for (int s = 0; s < kNumShards; s++)
                        shard_[s].Prune();
                }
                size_t TotalCharge() const override {// 计算总内存占用
                    size_t total = 0;
                    for (int s = 0; s < kNumShards; s++)
                        total += shard_[s].TotalCharge();
                    return total;
                }
        };
    }
    // 创建分片LRU缓存实例
    Cache* NewLRUCache(size_t capacity) { 
        return new ShardedLRUCache(capacity); 
    }
}