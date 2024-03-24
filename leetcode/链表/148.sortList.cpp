#include <iostream>
using namespace std;

// 难!!!
struct ListNode {
    int val;
    ListNode *next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}
};

class Solution {
private:
    ListNode* merge(ListNode* list1, ListNode* list2){//合并两个链表
        ListNode* pre_head = new ListNode(-1);
        ListNode* pre = pre_head;
        while(list1&&list2){
            if(list1->val>list2->val){
                pre->next=list2;
                list2=list2->next;
            }
            else{
                pre->next=list1;
                list1=list1->next;
            }
            pre = pre->next;
        }
        pre->next=list1==nullptr?list2:list1;
        return pre_head->next;
    }

public:
    ListNode* sortList(ListNode* head) {
        if(!head || !head->next)
            return head;
        ListNode* node = head;
        int length = 0;
        while(node){//统计长度
            length++;
            node = node->next;
        }
        ListNode* dummy = new ListNode(0, head);
        for(int sublength=1; sublength<length;sublength<<=1){//sublength表示当前子链表的长度
            ListNode* prev = dummy;
            ListNode* cur = dummy->next;
            while(cur){//每次遍历整个链表,将链表拆分成若干个长度为sublength的子链表,然后合并.这里才是真正的合并操作
            // 1 拆分成子链表
            ListNode* head_1 = cur;//第一个链表的头节点
            //找到第一个链表的尾节点,拆分出长度为sublength的链表1
            for(int i=1;i<sublength&&cur&&cur->next;i++)//i不能从0开始,因为对于sublength=1时的头节点和尾节点应该重合
                cur = cur->next;
            // 2 拆分sublength长度的链表2
            ListNode* head_2 = cur->next;
            cur->next = nullptr;//断开第一个子链表和第二个子链表的连接
            cur = head_2;
            //找第二个链表的尾节点,拆分出长度为sublength的链表2
            for(int i=1;i<sublength&&cur;i++)
                cur = cur->next;
            ListNode* nex = nullptr;// 3 第三个子链表的头节点(第3个子链表不用找尾节点)
            if(cur){//第二个子链表的尾节点可能为空,不为空时第三个链表才有头节点
                nex = cur->next;
                cur->next = nullptr;//断开第二个子链表和剩余链表的连接
            }
            // 4 合并两个sublength的有序子链表
            ListNode* merged = merge(head_1, head_2);
            prev->next = merged;
            // 5 移动prev至sublength*2的位置(下一次合并头节点的前一个节点),以连接下一次合并的结果
            while(prev->next)
                prev = prev->next;
            cur = nex;// 6 继续剩余链表的拆分和合并
            }
    }
    return dummy->next;
    }
};