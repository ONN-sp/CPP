//单调队列:利用deque容器(也可以利用vector容器)实现单调队列
#include<iostream>
#include<vector>
#include<deque>
using namespace std;
class Solution {
public:
    vector<int> maxSlidingWindow(vector<int>& nums, int k) {
        deque<int> q;
        vector<int> result;
        if(nums.size()<k)
            return result;
        for(int i=0;i<k;i++){//先处理第一个窗口
            while(!q.empty()&&nums[i]>q.back())
                q.pop_back();
            q.push_back(nums[i]);
        }
        result.emplace_back(q.front());
        for(int i=k;i<nums.size();i++){
            if(!q.empty()&&q.front()==nums[i-k])//只有当窗口第一个元素为此窗口最大值时才会pop_front,不然就会之前就被pop_back(为了保证队列是单调递减的,所以要在push元素前检查是否需要pop_back)
                q.pop_front();
            while(!q.empty()&&nums[i]>q.back())//为了保证单调队列,就要判断nums[i]和q.back()的大小关系
                q.pop_back();
            q.push_back(nums[i]);
            result.emplace_back(q.front());
        }
        return result;
    }
};