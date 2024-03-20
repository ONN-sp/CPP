#include <iostream>
using namespace std;

struct ListNode {
    int val;
    ListNode *next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}
};
//不用new创建新节点,直接使用原始链表修改,可能再新增几个节点
//这种方法的程序较复杂,考虑的细节多一点,但在执行用时上更快
class Solution {
public:
    ListNode* addTwoNumbers(ListNode* l1, ListNode* l2) {
        ListNode* p = l1;
        ListNode* pre = nullptr;
        int carry = 0;//进位值
        while(l1 && l2){            
            int sum = l1->val+l2->val+carry;         
            carry = sum / 10;
            l1->val = sum % 10;
            pre = l1;
            l1 = l1->next;
            l2 = l2->next;
        }
        while(l1){
            int sum = l1->val + carry;
            carry = sum / 10;
            l1->val = sum % 10;
            pre = l1;
            l1 = l1->next;
        }
        while(l2){
            pre->next = l2;
            int sum = l2->val + carry;
            carry = sum / 10;
            l2->val = sum % 10;
            pre = l2;
            l2 = l2->next;
        }
        if(carry > 0){
            ListNode* new_point = new ListNode(carry);
            pre->next = new_point;
        }
        return p;
    }
};
//时间复杂度:O(max(m,n))
//空间复杂度:O(1)

//利用new完全新建链表
//这种方法的程序很简洁
class Solution {
public:
    ListNode* addTwoNumbers(ListNode* l1, ListNode* l2) {
        ListNode* head=nullptr;
        ListNode* tail=nullptr;
        int carry=0;//进位值
        while(l1||l2){
            int n1=l1?l1->val:0;
            int n2=l2?l2->val:0;
            int sum=n1+n2+carry;
            if(!head)
                head=tail=new ListNode(sum%10);//head用来保存头节点,方便返回
            else{
                tail->next=new ListNode(sum%10);
                tail=tail->next;
            }
            carry=sum/10;
            if(l1)
                l1=l1->next;
            if(l2)
                l2=l2->next;
            }
        if(carry>0)
            tail->next=new ListNode(carry);
        return head;
    }
};
//时间复杂度:O(max(m,n))
//空间复杂度:O(1)(leetcode官方这样说的),但是需要用new分配新节点,从这方面考虑空间复杂度有O(max(n,m)).通常可以认为就是O(1)