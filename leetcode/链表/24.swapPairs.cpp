#include <iostream>
using namespace std;

struct ListNode {
    int val;
    ListNode *next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}
};

 //迭代法
class Solution {
public:
    ListNode* swapPairs(ListNode* head) {
        ListNode* cur=head;
        ListNode* dummyHead=new ListNode(0, head);//虚拟节点
        ListNode* temp=dummyHead;
        if(cur==nullptr || cur->next==nullptr)
            return head;
        while(temp->next&&temp->next->next){
            ListNode* node1=temp->next;
            ListNode* node2=temp->next->next;
            temp->next=node2;
            node1->next=node2->next;
            node2->next=node1;
            temp=node1;
        }
        return dummyHead->next;;
    }
};

class Solution {
public:
    ListNode* swapPairs(ListNode* head) {
        if(!head||!head->next)
            return head;
        ListNode* dummyhead = new ListNode(0);
        dummyhead->next = head; 
        ListNode* cur = dummyhead;
        while(cur->next&&cur->next->next){
            ListNode* temp1 = cur->next;
            ListNode* temp2 = cur->next->next->next;
            cur->next = cur->next->next;
            cur->next->next = temp1;
            cur->next->next->next = temp2;
            cur = cur->next->next;
        }
        return dummyhead->next;
    }
};
//时间复杂度：O(n)
//空间复杂度：O(1)

//递归法
class Solution {
public:
    ListNode* swapPairs(ListNode* head) {
        if(!head || !head->next)
            return head;
        ListNode* node1=head;
        ListNode* node2=head->next;
        ListNode* node3=head->next->next;
        node1->next = swapPairs(node3);
        node2->next = node1;
        return node2;
    }
};
//时间复杂度：O(n)
//空间复杂度：O(n)