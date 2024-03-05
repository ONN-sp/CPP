#include<iostream>
#include<vector>
using namespace std;

//动态规划
class Solution {
public:
    int maxSubArray(vector<int>& nums){
        vector<int> dp(nums.size());//确定dp数组
        dp[0]=nums[0];//初始化
        int result=dp[0];
        //下标含义:此题目中的dp数组第i个元素就表示i及i以前位置连续序列的最大和值
        if(nums.size()==0) return result;
        for(int i=1;i<nums.size();i++){//确定遍历顺序
            dp[i]=max(nums[i]+dp[i-1], nums[i]);//确定递推公式
            result=max(dp[i],result);
        }
        return result;
    }
};