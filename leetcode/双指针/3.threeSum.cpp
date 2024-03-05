//这个题最重要的是去重,但这里的去重不是去掉数组的相同元素,而是去掉相同结果集,所有set不能直接完成这里的去重
//排序+双指针法,细节为去重
#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;

class Solution {
public:
    vector<vector<int>> threeSum(vector<int>& nums){
        vector<vector<int>> result;
        if(nums.size()<3)
            return result;
        sort(nums.begin(),nums.end());//先排序
        for(int i=0;i<nums.size();i++){
            int forward=i+1;
            int end=nums.size()-1;
            if(nums[i]>0)
                return result;
            if(i>0&&nums[i]==nums[i-1])//去重nums[i],不能是nums[i+1](这是去掉数组的重复元素不是这个题的去重)
                continue;
            while(forward<end){
                if(nums[i]+nums[forward]+nums[end]==0){
                    result.emplace_back(vector<int>{nums[i],nums[forward],nums[end]});
                    while(forward<end&&nums[forward]==nums[forward+1])//去重nums[forward]
                        forward++;
                    while(forward<end&&nums[end]==nums[end-1])//去重nums[end]
                        end--;
                    forward++;//必须放在nums[forward]去重的后面
                    end--;//必须放在nums[end]去重的后面
                    }
                else if(nums[i]+nums[forward]+nums[end]>0){
                    end--;
                }
                else{
                    forward++;
                }
            }
        }
        return result;
    }
};