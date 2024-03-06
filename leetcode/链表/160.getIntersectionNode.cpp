#include <iostream>
using namespace std;


 //Definition for singly-linked list.
 struct ListNode {
     int val;
     ListNode *next;
     ListNode(int x) : val(x), next(NULL) {}
};

class Solution {
public:
    ListNode *getIntersectionNode(ListNode *headA, ListNode *headB) {
        ListNode* cura=headA;
        ListNode* curb=headB;
        int size_A=1;
        int size_B=1;
        while(cura->next!=NULL){//获取链表A的长度
            size_A++;
            cura=cura->next;
        }
        cout << size_A << endl;
        cura=headA;
        while(curb->next!=NULL){//获取链表B的长度
            size_B++;
            curb=curb->next;
        }
        curb=headB;
        //把长的链表移动到尾部对齐的形式的节点处
        if(size_A>size_B)
            for(int i=0;i<size_A-size_B;i++)
                cura=cura->next;
        else if(size_A<size_B)
            for(int i=0;i<size_B-size_A;i++)
                curb=curb->next;
        while(cura!=curb){//找相交节点
            cura=cura->next;
            curb=curb->next;
        }
        return cura;
    }
};