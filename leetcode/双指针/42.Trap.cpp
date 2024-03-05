//双指针法
//按列计算   每一列的积水面积的公式=(min(left_max,right_max)-height[i])*1
#include<iostream>
#include<vector>
using namespace std;

class Solution {
public:
    int trap(vector<int>& height) {
         int left=0;//对撞指针
         int right=height.size()-1;//对撞指针
         int left_max=0;
         int right_max=0;
         int result=0;
         while(left<right){
             if(height[left]<=height[right]){//此时算低洼的积水面积就只需要知道left_max
                 if(height[left]<left_max)//形成低洼
                    result+=(left_max-height[left])*1;//此列的积水面积
                else
                    left_max=height[left];
                left++;
             }
             else{
                 if(height[right]<right_max)//形成低洼
                     result+=(right_max-height[right])*1;//此列的积水面积
                 else
                     right_max=height[right];
                 right--;
             }          
         }   
         return result;                
    }
};