#include <iostream>
using namespace std;

struct ListNode {
     int val;
     ListNode *next;
     ListNode(int x) : val(x), next(NULL) {}
 };

//快慢指针
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