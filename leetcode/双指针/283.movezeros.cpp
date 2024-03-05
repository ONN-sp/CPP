//双指针法
#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;

class Solution {
public:
    void moveZeroes(vector<int>& nums) {
        int* p1;
        int* p2;
        p1 = &nums[0];
        p2 = p1;
        for(int i=0;i<nums.size();i++){
            for(int j=i+1;j<nums.size();j++){
            p2++;
            if(!(*p1))
                swap(*p1, *p2);  
            }
            p1++; 
            p2=p1;       
        }
    }
};