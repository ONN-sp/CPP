#include<iostream>
#include<vector>
using namespace std;

//暴力搜索
class Solution {
public:
    vector<int> twoSum(vector<int>& nums, int target) {
        for(int i = 0; i < nums.size(); i++){
            for(int j = i+1; j < nums.size(); j++){
                if(nums[i]+nums[j] == target){
                    vector<int> v{i,j};
                    return v;
                 }
            }
        }
        vector<int> v{0,0};
        return v;
    }
};
// unordered_map哈希表查找(O(1)查找复杂度)
// 找target-nums[i]是在已加入哈希表的<key, value>中寻找,即3 2 4是在访问到nums[2]=4中才确定的
#include<unordered_map>
class Solution {
public:
    vector<int> twoSum(vector<int>& nums, int target) {
        unordered_map<int, int> m;
        for(int i = 0;i<nums.size();i++){
            int find = target - nums[i];
            if(m.count(find)!=0){
                vector<int> v = {i, m[find]};
                return v;
            }
            m[nums[i]] = i;
        }
        vector<int> v = {0, 0};
        return v;
    }
};