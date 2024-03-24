#include <iostream>
using namespace std;

struct ListNode {
    int val;
    ListNode *next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}
};

class Solution {
private:
    ListNode* reverse(ListNode* cur){
        ListNode* pre = nullptr;
        while(cur){
            ListNode* nex=cur->next;
            cur->next=pre;
            pre=cur;
            cur=nex;
        }
        return pre;
    }
public:
    ListNode* reverseKGroup(ListNode* head, int k) {
        ListNode* dummy = new ListNode(0, head);
        ListNode* start = dummy;
        ListNode* end = dummy;
        while(1){
            for(int i=0;i<k && end;i++)
                end = end->next;  
            if(!end)
                break;    
            //startNext和endNext是关键,保存当前组的末尾节点和下一组开始的初始节点.这是为了实现前一组和后一组节点的连接
            ListNode* startNext = start->next;
            ListNode* endNext = end->next;
            //断开当前组的链表节点,将末尾节点next设为nullptr,便于reverse
            end->next = nullptr;
            start->next = reverse(start->next);
            startNext->next = endNext;
            start = end = startNext;       
        }
        return dummy->next;
    }
};