#include <iostream>
#include <unordered_map>
using namespace std;

class Node {
public:
    int val;
    Node* next;
    Node* random;
    
    Node(int _val) {
        val = _val;
        next = NULL;
        random = NULL;
    }
};

//迭代+节点拆分
class Solution {
public:
    Node* copyRandomList(Node* head) {
        if(head==nullptr)
            return nullptr;
        Node* dummy = new Node(0);
        Node* cur = head;
        while(cur){//将新节点依次插在源节点后面
            Node* new_node = new Node(cur->val);
            new_node->next = cur->next;
            cur->next = new_node;
            cur = cur->next->next;
        }
        cur = head;
        while(cur){//改变新节点的random指针
            if(cur->random)
                cur->next->random = cur->random->next;
            else
                cur->next->random = NULL;
            cur = cur->next->next;
        }
        dummy->next = head->next;
        cur = head;
        while(cur->next){//改变源节点的next和新节点的next指针
            Node* temp = cur->next;
            cur->next = cur->next->next;
            cur = temp;
        }
        return dummy->next;
    }
};
//时间复杂度：O(n)
//空间复杂度：O(1)

//哈希+回溯
class Solution {
public:
    unordered_map<Node*, Node*> m;
    Node* copyRandomList(Node* head) {
        if(!head)
            return nullptr;
        if(!m.count(head)){
            Node* new_head = new Node(head->val);
            m[head] = new_head; 
            new_head->next = copyRandomList(head->next);
            new_head->random = copyRandomList(head->random);
        }
        return m[head];
    }
};
//时间复杂度：O(n)
//空间复杂度：O(n)