#include <iostream>
#include <vector>
#include <queue>
using namespace std;

struct ListNode {
    int val;
    ListNode *next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}
};

//顺序合并
class Solution {
private:
    ListNode* mergeTwoLists(ListNode* list1, ListNode* list2){
        ListNode* pre_head = new ListNode(-1);//虚拟头节点
        ListNode* pre = pre_head;
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
        pre = pre_head->next;
        delete pre_head;
        return pre;
    }
public:
    ListNode* mergeKLists(vector<ListNode*>& lists) {  
        if(lists.size()==0)
            return nullptr;
        ListNode* ans = lists[0];   
        for(int i=1;i<lists.size();i++){
            ans = mergeTwoLists(ans, lists[i]);
        }
    return ans;
    }
};
//时间复杂度：O(k^2n)
//空间复杂度：O(1)

//分治合并
class Solution {
private:
    ListNode* mergeTwoLists(ListNode* list1, ListNode* list2){
        ListNode* pre_head = new ListNode(-1);//虚拟头节点
        ListNode* pre = pre_head;
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
        pre = pre_head->next;
        delete pre_head;
        return pre;
    }
private:
    ListNode* merge(vector<ListNode*> &lists, int start, int end){
        int m = (end+start)/2;
        if(start==end)
            return lists[start];
        if(start > end)//处理空链表数组
            return nullptr;
        auto left = merge(lists, start, m);
        auto right = merge(lists, m+1, end);
        ListNode* res = mergeTwoLists(left, right);
        return res;
    }
public:
    ListNode* mergeKLists(vector<ListNode*>& lists) {
        return merge(lists, 0, lists.size()-1);
    }
};
//时间复杂度：O(kn*logk)
//空间复杂度：O(logk)

//优先队列合并
class Solution {
public:
    struct Status{
        int val;
        ListNode* ptr;
        bool operator<(const Status& rhs) const{//重载运算符
            return val>rhs.val;
        }
    };
    priority_queue<Status> q;
    ListNode* mergeKLists(vector<ListNode*>& lists) {
        for(auto list:lists){
            if(list)
                q.push({list->val, list});  
                }
        ListNode head;
        ListNode* cur = &head;//指针要先分配地址,再赋值
        while(!q.empty()){
            auto temp = q.top();
            q.pop();
            cur->next = temp.ptr;
            cur = cur->next;
            if(temp.ptr->next)
                q.push({temp.ptr->next->val, temp.ptr->next});
        }
        return head.next;
    }
};
//时间复杂度：O(kn*logk)
//空间复杂度：O(k)