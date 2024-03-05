//双指针法
#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;

class Solution {
public:
    int maxArea(vector<int>& height) {
        int forward;//前下标
        int end;//尾下标
        int result=0;
        forward = 0;
        end = height.size()-1;
        while(forward!=end){
                int temp = min(height[forward], height[end])*(end-forward);
                result=max(temp, result);
            if(height[forward]<height[end]){
                forward++;
            }
            else{
                end--;
            }
        }
        return result;
    }
};