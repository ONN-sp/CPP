#include <vector>
#include <algorithm>
using namespace std; 

class Solution {
public:
    vector<vector<int>> fourSum(vector<int>& nums, int target) {
        vector<vector<int>> res;
        sort(nums.begin(), nums.end());
        for(int k=0;k<nums.size();k++){// 多加一层循环
            if(nums[k]>target&&target>0)// nums[k]剪枝
                break;
            if(k>0&&nums[k]==nums[k-1])// nums[k]去重
                continue;
            for(int i=k+1;i<nums.size();i++){
                if(nums[k]+nums[i]>target&&target>0)// nums[i]剪枝
                    break;
                if(i>k+1&&nums[i]==nums[i-1])// 这里不是i>0 !!!
                    continue;
                int left=i+1;
                int right=nums.size()-1;
                while(left<right){
                    if((long long)nums[k]+nums[i]+nums[left]+nums[right]==target){
                        res.emplace_back(vector<int>{nums[k],nums[i],nums[left],nums[right]});
                        while(left<right&&nums[left]==nums[left+1])
                            left++;
                        while(left<right&&nums[right]==nums[right-1]) 
                            right--;
                        left++;
                        right--;
                    }
                    else if((long long) nums[k]+nums[i]+nums[left]+nums[right]<target)
                        left++;
                    else
                        right--;
                }
            }
        }
        return res;
    }
};