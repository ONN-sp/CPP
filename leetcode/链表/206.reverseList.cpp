#include <iostream>
using namespace std;


// Definition for singly-linked list.
struct ListNode {
     int val;
     ListNode *next;
     ListNode() : val(0), next(nullptr) {}
     ListNode(int x) : val(x), next(nullptr) {}
     ListNode(int x, ListNode *next) : val(x), next(next) {}
};

//双指针法(此方法设定一个nullptr,让它在翻转指针时一起参与赋值,这样就不用在最后赋nullptr了)
class Solution {
public:
    ListNode* reverseList(ListNode* head) {
        if(head==nullptr || head->next==nullptr)
            return head;
        ListNode* cur=head;
        ListNode* pre=nullptr;
        while(cur!=nullptr){//移至末尾 
            ListNode* nex=cur->next;          
            cur->next=pre;
            pre=cur;
            cur=nex;     
        }
        return pre;
      
    }
};
//时间复杂度:O(n)
//空间复杂度:O(1)

 //递归法
class Solution {
public:
    ListNode* reverseList(ListNode* head) {
        if(head==nullptr||head->next==nullptr)
            return head;
        return recur(head, nullptr);

    }
private:
    ListNode* recur(ListNode* node, ListNode* pre){
        if(node==nullptr){
            return pre;
        }
        ListNode* res = recur(node->next, node);
        node->next=pre;
        return res;
    }
};
//时间复杂度:O(n)
//空间复杂度:O(n)
