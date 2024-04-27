#include <iostream>
#include <unordered_map>
using namespace std;

//双向链表(O(1)实现删除节点和将访问过的添加到头节点)+哈希表(O(1)复杂度随机访问节点)

//不借用list容器,自建一个双向链表
struct Node{
    int key;
    int val;
    Node* pre;
    Node* next;
    Node():key(0), val(0), pre(nullptr), next(nullptr){}
    Node(int _key, int _val):key(_key), val(_val), pre(nullptr), next(nullptr){}
};

class LRUCache {
private:
    Node* head, *tail;//这里的头节点和尾节点其实就是相当于两个虚拟节点即dummy_node,没有包括数据,只是为了方便删除、添加节点
    unordered_map<int, Node*> h;
    int capacity, size;
public:
    LRUCache(int _capacity) : capacity(_capacity), size(0) {
        head = new Node();
        tail = new Node();
        head->next = tail;
        tail->next = head;
    }
    
    int get(int key) {
        if(h.count(key)){
            removeNode(h[key]);
            addNodeTohead(h[key]);
            return h[key]->val;
        }
        return -1;
    }
    
    void put(int key, int value) {
        if(h.count(key)){
            h[key]->val = value;
            removeNode(h[key]);
            addNodeTohead(h[key]);
        }
        else{
            if(size==capacity){
                h.erase(tail->pre->key);
                removeNode(tail->pre);
                size--;
            }
            Node* node = new Node(key, value);
            addNodeTohead(node);
            h[key] = node;
            size++;
        }
    }

    //从链表中删除节点
    void removeNode(Node* node){
        node->pre->next = node->next;
        node->next->pre = node->pre;
    }

    //访问过的键值对给它移到头节点处
    void addNodeTohead(Node* node){
        node->pre = head;
        node->next = head->next;
        head->next->pre = node;
        head->next = node;
    }
};

/*
测试程序:
LRUCache lRUCache = new LRUCache(2);
lRUCache.put(1, 1); // 缓存是 {1=1}
lRUCache.put(2, 2); // 缓存是 {1=1, 2=2}
lRUCache.get(1);    // 返回 1
lRUCache.put(3, 3); // 该操作会使得关键字 2 作废，缓存是 {1=1, 3=3}
lRUCache.get(2);    // 返回 -1 (未找到)
lRUCache.put(4, 4); // 该操作会使得关键字 1 作废，缓存是 {4=4, 3=3}
lRUCache.get(1);    // 返回 -1 (未找到)
lRUCache.get(3);    // 返回 3
lRUCache.get(4);    // 返回 4
*/