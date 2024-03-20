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
public:
    ListNode* mergeTwoLists(ListNode* list1, ListNode* list2) {
        ListNode* prehead = new ListNode(-1);//虚拟头节点
        ListNode* pre = prehead;//不能一上来就把pre设为list1或list2,这样会让下面陷入死循环    
        while(list1!=nullptr && list2!=nullptr){
            if(list1->val > list2->val){
                pre->next = list2;
                list2 = list2->next;
            }
            else{
                pre->next = list1;
                list1 = list1->next;
            }
            pre = pre->next;
        }
        
        pre->next = list2==nullptr?list1:list2;
        pre = prehead->next;
        delete prehead;
        return pre;
    }
};
//时间复杂度:O(m+n)
//空间复杂度:O(1)