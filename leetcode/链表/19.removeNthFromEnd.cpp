#include <iostream>
#include <unordered_map>
using namespace std;
//哈希表法
struct ListNode {
    int val;
    ListNode *next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}
};

class Solution {
public:
    ListNode* removeNthFromEnd(ListNode* head, int n) {
        unordered_map<int, ListNode*> m;
        ListNode* p = head;
        int count = 0;
        while(p){
            m[count] = p;
            count++;
            p = p->next;
        }
        if(n > count || count==1)
            return nullptr;
        else{
            if(count==n){
                head = head->next;
                delete m[count-n];//删除倒数第n个节点
            }
            else if(n==1){
                delete m[count-n];
                m[count-n-1]->next = nullptr;
            }
            else{
                delete m[count-n];//删除倒数第n个节点
                m[count-n-1]->next = m[count-n+1];
            }
        }
        return head;

    }
};
//时间复杂度:O(n)
//空间复杂度:O(n)

//双指针法
class Solution {
public:
    ListNode* removeNthFromEnd(ListNode* head, int n) {
        ListNode* first=head;
        ListNode* dummy = new ListNode(0, head);//虚拟头节点
        ListNode* second = dummy;
        for(int i=0;i<n;i++)
            first=first->next;
        //此时first比second超前n步
        while(first){
            first=first->next;
            second=second->next;
        }
        //此时second在倒数第n个节点的前一个节点
        second->next=second->next->next;
        return dummy->next;
    }
};
//时间复杂度:O(n)
//空间复杂度:O(1)