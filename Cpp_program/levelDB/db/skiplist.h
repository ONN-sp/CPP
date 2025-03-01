#ifndef STORAGE_LEVELDB_DB_SKIPLIST_H_
#define STORAGE_LEVELDB_DB_SKIPLIST_H_

#include <atomic>
#include <cassert>
#include <cstdlib>
#include "../util/arena.h"
#include "../util/random.h"

namespace leveldb {
    /**
     * @brief 跳表（SkipList）模板类
     * 
     * @tparam Key 
     * @tparam Comparator 
     */
    template <typename Key, class Comparator>
    class SkipList {
        private:
            struct Node;// 跳表节点结构
        public:
            explicit SkipList(Comparator cmp, Arena* arena);
            // 禁止拷贝构造和赋值操作
            SkipList(const SkipList&) = delete;
            SkipList& operator=(const SkipList&) = delete;
            // 插入键到跳表中（线程安全）
            void Insert(const Key& key);
            // 检查键是否存在（线程安全）
            bool Contains(const Key& key) const;
            /**
             * @brief 跳表迭代器类，提供遍历功能
             * 
             */
            class Iterator {
                public:
                    explicit Iterator(const SkipList* list);
                    bool Valid() const;// 判断当前迭代位置是否有效
                    const Key& key() const;// 获取当前节点的键（仅在Valid()为true时调用）
                    void Next();// 移动到下一个节点（正向遍历）
                    void Prev();// 移动到前一个节点（逆向遍历，低效）
                    void Seek(const Key& target);// 定位到第一个>=target的节点
                    void SeekToFirst();// 定位到首节点（最小键）
                    void SeekToLast();// 定位到尾节点（最大键）
                private:
                    const SkipList* list_;
                    Node* node_;
            };
        private:
            enum { kMaxHeight = 12 };// 最大层数限制，平衡空间与查询效率
            inline int GetMaxHeight() const {// 获取当前跳表实际使用的最大层数
                return max_height_.load(std::memory_order_relaxed);
            }
            Node* NewNode(const Key& key, int height);// 创建新节点，分配内存并构造对象（使用Arena分配器）
            int RandomHeight();// 生成随机层数（概率为1/4的指数分布）
            bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }// 判断两个键是否相等
            bool KeyIsAfterNode(const Key& key, Node* n) const;// 判断key是否在节点n之后（比较结果是否n->key < key）
            Node* FindGreaterOrEqual(const Key& key, Node** prev) const;// 查找第一个>=key的节点，并记录各层的前驱节点到prev数组
            Node* FindLessThan(const Key& key) const;// 查找最后一个<key的节点（用于逆向遍历）
            Node* FindLast() const;// 查找跳表中最后一个节点（最大键）
            Comparator const compare_;// 键比较器实例
            Arena* const arena_;// 内存分配器，负责节点内存管理
            Node* const head_;// 头节点，不存储数据，各层的起始点
            std::atomic<int> max_height_;// 当前使用的最大层数
            Random rnd_;// 随机数生成器，用于决定节点层数
    };
    /**
     * @brief 跳表节点结构体定义
     * 
     * @tparam Key 
     * @tparam Comparator 
     */
    template <typename Key, class Comparator>
    struct SkipList<Key, Comparator>::Node {
        explicit Node(const Key& k) : key(k) {}
        Key const key;// 节点存储的键
        // 获取第n层的下一个节点（带内存屏障，保证读取顺序）
        Node* Next(int n) {
            assert(n >= 0);
            return next_[n].load(std::memory_order_acquire);
        }
         // 设置第n层的下一个节点（带内存屏障，保证写入可见性）
        void SetNext(int n, Node* x) {
            assert(n >= 0);
            next_[n].store(x, std::memory_order_release);
        }
        // 无屏障版本，用于单线程或已知线程安全的环境
        Node* NoBarrier_Next(int n) {
            assert(n >= 0);
            return next_[n].load(std::memory_order_relaxed);
        }
        void NoBarrier_SetNext(int n, Node* x) {
            assert(n >= 0);
            next_[n].store(x, std::memory_order_relaxed);
        }
    private:
        std::atomic<Node*> next_[1];// 实际大小在创建时确定，存储各层的下一个节点指针
    };
    /**
     * @brief 创建新节点
     * 
     * @tparam Key 
     * @tparam Comparator 
     * @param key 
     * @param height 
     * @return SkipList<Key, Comparator>::Node* 
     */
    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::NewNode(const Key& key, int height) {
        // 计算总内存：节点结构 + (height-1)个next_元素（柔性数组扩展）
        char* const node_memory = arena_->AllocateAligned(sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
        return new (node_memory) Node(key);// 就地构造Node对象
    }
    template <typename Key, class Comparator>
    inline SkipList<Key, Comparator>::Iterator::Iterator(const SkipList* list) {
        list_ = list;
        node_ = nullptr;
    }
    template <typename Key, class Comparator>
    inline bool SkipList<Key, Comparator>::Iterator::Valid() const {
        return node_ != nullptr;// 非空即为有效
    }
    template <typename Key, class Comparator>
    inline const Key& SkipList<Key, Comparator>::Iterator::key() const {
        assert(Valid());
        return node_->key;// 直接返回节点键
    }
    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::Next() {
        assert(Valid());
        node_ = node_->Next(0);// 第0层为全链表，直接顺序后移
    }
    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::Prev() {
        assert(Valid());
        node_ = list_->FindLessThan(node_->key);// 逆向查找需要重新搜索，时间复杂度O(n)
        if (node_ == list_->head_)// 若找到头节点，设为无效
            node_ = nullptr;
    }
    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::Seek(const Key& target) {
        node_ = list_->FindGreaterOrEqual(target, nullptr);// 查找第一个>=target的节点
    }
    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::SeekToFirst() {
        node_ = list_->head_->Next(0);// 头节点的第0层下一个即为最小元素
    }
    template <typename Key, class Comparator>
    inline void SkipList<Key, Comparator>::Iterator::SeekToLast() {
        node_ = list_->FindLast();// 跳表尾部
    if (node_ == list_->head_)// 空表情况处理
        node_ = nullptr;
    }
    /**
     * @brief 生成随机层数
     * 
     * @tparam Key 
     * @tparam Comparator 
     * @return int 
     */
    template <typename Key, class Comparator>
    int SkipList<Key, Comparator>::RandomHeight() {
        static const unsigned int kBranching = 4;// 每层概率为1/4
        int height = 1;
        while (height < kMaxHeight && rnd_.OneIn(kBranching))// 循环直到达到kMaxHeight或随机失败
            height++;
        assert(height > 0);
        assert(height <= kMaxHeight);
        return height;
    }
    template <typename Key, class Comparator>
    bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
        return (n != nullptr) && (compare_(n->key, key) < 0);// 判断key是否在节点n之后
    }
    /**
     * @brief 找到第一个>=key的节点，并记录路径
     * 
     * @tparam Key 
     * @tparam Comparator 
     * @param key 
     * @param prev 
     * @return SkipList<Key, Comparator>::Node* 
     */
    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node*
    SkipList<Key, Comparator>::FindGreaterOrEqual(const Key& key, Node** prev) const {
        Node* x = head_;// 从头节点开始
        int level = GetMaxHeight() - 1;// 从最高层开始查找
        while (true) {
            Node* next = x->Next(level);// 当前层的下一个节点
            if (KeyIsAfterNode(key, next))// next->key < key
                x = next;// 在本层继续前进
            else {// 记录该层的前驱节点（用于插入时维护链表）
                if (prev != nullptr) 
                    prev[level] = x;
                if (level == 0)// 到达底层，返回结果
                    return next;
                else 
                    level--;// 下一层继续查找
            }
        }
    }
    /**
     * @brief 查找最后一个小于key的节点 
     * 
     * @tparam Key 
     * @tparam Comparator 
     * @param key 
     * @return SkipList<Key, Comparator>::Node* 
     */
    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node*
    SkipList<Key, Comparator>::FindLessThan(const Key& key) const {
        Node* x = head_;
        int level = GetMaxHeight() - 1;
        while (true) {
            assert(x == head_ || compare_(x->key, key) < 0);
            Node* next = x->Next(level);
            if (next == nullptr || compare_(next->key, key) >= 0) {
                if (level == 0)// 到底层返回当前节点
                    return x;
                else
                    level--;// 向下层继续
            } 
            else 
                x = next; // 在本层前进   
        }
    }
    /**
     * @brief 查找跳表的最后一个节点（最大键）
     * 
     * @tparam Key 
     * @tparam Comparator 
     * @return SkipList<Key, Comparator>::Node* 
     */
    template <typename Key, class Comparator>
    typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLast() const {
        Node* x = head_;
        int level = GetMaxHeight() - 1;
        while (true) {
            Node* next = x->Next(level);
            if (next == nullptr) {// 当前层已无节点
                if (level == 0) // 到底层返回当前节点
                    return x;
                else
                    level--;// 向下一层查找
            } 
            else
                x = next;// 本层存在后续节点，继续前进
        }
    }
    template <typename Key, class Comparator>
    SkipList<Key, Comparator>::SkipList(Comparator cmp, Arena* arena)
        : compare_(cmp),
        arena_(arena),
        head_(NewNode(0 /* any key will do */, kMaxHeight)),
        max_height_(1),
        rnd_(0xdeadbeef) 
    {
        for (int i = 0; i < kMaxHeight; i++)// 初始化头节点各层的next指针为空
            head_->SetNext(i, nullptr);
    }
    /**
     * @brief 插入键到跳表中
     * 
     * @tparam Key 
     * @tparam Comparator 
     * @param key 
     */
    template <typename Key, class Comparator>
    void SkipList<Key, Comparator>::Insert(const Key& key) {
        Node* prev[kMaxHeight];// 记录各层的前驱节点
        Node* x = FindGreaterOrEqual(key, prev);// 查找插入位置
        assert(x == nullptr || !Equal(key, x->key));// 确保无重复
        int height = RandomHeight();// 生成新节点层数
        if (height > GetMaxHeight()) {// 如果新节点层数超过当前最大层，补充prev数组
            for (int i = GetMaxHeight(); i < height; i++)
                prev[i] = head_;// 高层前驱只能是头节点
            max_height_.store(height, std::memory_order_relaxed);// 原子更新最大层数（无需锁，其他线程会逐渐看到更新）
        }
        x = NewNode(key, height);// 创建新节点
        // 逐层插入新节点
        for (int i = 0; i < height; i++) {
            x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));// 无屏障设置next指针（当前线程独占插入，其他线程通过FindGreaterOrEqual的屏障保证可见性）
            prev[i]->SetNext(i, x);// 前驱节点设置next（带屏障，确保新节点对后续线程可见）
        }
    }
    /**
     * @brief 检查键是否存在
     * 
     * @tparam Key 
     * @tparam Comparator 
     * @param key 
     * @return true 
     * @return false 
     */
    template <typename Key, class Comparator>
    bool SkipList<Key, Comparator>::Contains(const Key& key) const {
        Node* x = FindGreaterOrEqual(key, nullptr);
        if (x != nullptr && Equal(key, x->key))
            return true;
        else 
            return false;
    }
}
#endif