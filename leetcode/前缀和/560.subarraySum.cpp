#include<iostream>
#include<vector>
#include<unordered_map>
using namespace std;

//这个题用滑动窗口法感觉可以，但是我没成功，因为窗口有点不好取，不好判断怎样才构成一个窗口
//前缀和+哈希表
class Solution {
public:
    int subarraySum(vector<int>& nums, int k) {
        int result=0;
        int sum_temp=0;
        unordered_map<int, int> mm;//key=前缀和,value=这个前缀和出现的次数
        mm[0]=1;
        for(auto& num:nums){
            sum_temp=sum_temp+num;
            if(mm.count(sum_temp-k)){
                result=result+mm[sum_temp-k];
            }
            mm[sum_temp]++;
        }
        return result;
    }
};