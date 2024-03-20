#include <iostream>
using namespace std;

struct ListNode {
     int val;
     ListNode *next;
     ListNode(int x) : val(x), next(NULL) {}
 };

//快慢指针
//slow=head;fast=head->next
////此时a=c+(n-1)(b+c)-1
class Solution {
public:
    ListNode *detectCycle(ListNode *head) {
        if(head==nullptr || head->next==nullptr)
            return NULL;
        ListNode * slow=head;
        ListNode * fast=head->next;
        while(slow!=fast){
            if(fast==nullptr || fast->next==nullptr)
                return NULL;
            slow=slow->next;
            fast=fast->next->next;
        }
        ListNode* ptr=head;
        while(slow->next!=ptr){
            slow=slow->next;
            ptr=ptr->next;
        }
        return ptr;
    }
};

//slow和fast指针都从head开始
//此时a=c+(n-1)(b+c)
class Solution {
public:
    ListNode *detectCycle(ListNode *head) {
        if(head==nullptr || head->next==nullptr)
            return NULL;
        ListNode * slow=head;
        ListNode * fast=head;
        while(fast!=nullptr){//为了排除那些没有环的链表
            if(fast->next==nullptr)
                return NULL;
            slow=slow->next;
            fast=fast->next->next;
            if(slow==fast){
                ListNode* ptr=head;
                while(slow!=ptr){
                    slow=slow->next;
                    ptr=ptr->next;
                }
                return ptr;
            }
        }
        return NULL;
    }
};