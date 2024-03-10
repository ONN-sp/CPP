#include <iostream>
using namespace std;

struct ListNode {
     int val;
     ListNode *next;
     ListNode() : val(0), next(nullptr) {}
     ListNode(int x) : val(x), next(nullptr) {}
     ListNode(int x, ListNode *next) : val(x), next(next) {}
};


//递归法(很耗栈内存,效率不高)
class Solution {
public:
    bool isPalindrome(ListNode* head) {
        ListNode* left = head;
        left = recura(head, nullptr, left);
        if(left==nullptr)
            return true;
        return false;
    }
private:
    ListNode* recura(ListNode* cur, ListNode* pre, ListNode* left){
        if(cur==nullptr)
            return left;
        left = recura(cur->next,cur,left);
        if(cur->val!=left->val)
            return left;
        left = left->next;
        return left;
    }
};




 //快慢指针
class Solution {
public:
    bool isPalindrome(ListNode* head) {
        ListNode* fast=head;
        ListNode* slow=head;
        //找到中间节点
        while(fast!=nullptr&&fast->next!=nullptr){//偶数链表为fast!=nullptr,奇数链表为fast->next!=nullptr
            fast=fast->next->next;
            slow=slow->next;
        }
        if(fast!=nullptr)//奇数链表要单独处理一下
            slow=slow->next;
        //反转后一半的链表
        slow=reverse(slow);
        while(slow!=nullptr){
            if(slow->val!=head->val)
                return false;
            slow=slow->next;
            head=head->next;
        }
        return true;
    }
private:
    ListNode* reverse(ListNode* cur){
        ListNode* pre=nullptr;
        while(cur!=nullptr){
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