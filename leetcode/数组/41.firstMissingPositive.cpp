#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

//基于哈希表在数组上用num--index构成的"哈希表"
class Solution {
public:
    int firstMissingPositive(vector<int>& nums) {
        for(int i=0;i<nums.size();i++){
            if(nums[i]<=0)
                nums[i]=nums.size()+1;//第一步,将所有小于等于0的数设为nums.size()+1,因为这些数在算最小正整数时其实是不用遍历的
        }
        for(int i=0;i<nums.size();i++){
            if(abs(nums[i])<=nums.size()&&nums[abs(nums[i])-1]>0)
                nums[abs(nums[i])-1]=-nums[abs(nums[i])-1];//第二步,小于等于nums.size()的数取相反数
        }
        for(int i=0;i<nums.size();i++){
            if(nums[i]>0)//第三步,最小的正数的index+1就是最终结果
                return i+1;
        }
        return nums.size()+1;
    }
};

//置换法
class Solution {
public:
    int firstMissingPositive(vector<int>& nums) {
        for(int i=0;i<nums.size();i++){
            while(nums[i]<=nums.size()&&nums[i]>0&&nums[i]!=nums[nums[i]-1])//必须是while,不能是if
                swap(nums[i], nums[nums[i]-1]);
        }
        for(int i=0;i<nums.size();i++)
            if(nums[i]!=i+1)//！！！
                return i+1;
        return nums.size()+1;
    }
};