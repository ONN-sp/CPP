//双指针法
//输入是有序的数组
#include<iostream>
#include<vector>
using namespace std;
class Solution {
public:
    vector<int> twoSum(vector<int>& numbers, int target) {
        vector<int> result;
        int end=numbers.size()-1;
        int forward = 0;
        if(numbers.size()<2)//不可行剪枝
            return result;
        if(numbers[0]>0&&numbers[0]>target)//不可行剪枝
                return result;
            while(forward<end){
                if(numbers[forward]+numbers[end]==target){
                    result.emplace_back(forward+1);
                    result.emplace_back(end+1);
                    return result;//因为只有一种解,所以找到可以直接返回
            }
            else if(numbers[forward]+numbers[end]>target)
                end--;
            else
                forward++;
        }
    return result;
    }
};