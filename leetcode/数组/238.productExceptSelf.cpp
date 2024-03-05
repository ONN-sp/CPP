#include<iostream>
#include<vector>
using namespace std;

//左右列表法,前缀积列表、后缀积列表
class Solution {
public:
    vector<int> productExceptSelf(vector<int>& nums) {
        vector<int> result;
        //保存前缀积到result数组
        for(int i=0;i<nums.size();i++){
            if(i==0)
                result.emplace_back(1);
            else
                result.emplace_back(nums[i-1]*result.back());
        }
        //计算后缀积,并乘到对应的result数组中
        for(int i=nums.size()-1,temp;i>-1;i--){
            if(i==nums.size()-1){
                temp=1;
                result[i]=result[i]*temp;       
            }
            else{
                temp=temp*nums[i+1];
                result[i]=result[i]*temp;
        }
    }
    return result;
    }
};