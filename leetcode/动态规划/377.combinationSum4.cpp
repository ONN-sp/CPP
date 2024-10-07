#include <iostream>
#include <vector>
using namespace  std;

class Solution {
public:
    // 排列问题:物品遍历在内层
int combinationSum4(vector<int>& nums, int target){
    vector<double> dp(target+1);
    dp[0] = 1;
    for(int j=0;j<=target;j++){// 容量遍历在外层
        for(int i=0;i<nums.size();i++)
            if(j>=nums[i])
                dp[j] += dp[j-nums[i]];
    }
    return dp[target];
}
};