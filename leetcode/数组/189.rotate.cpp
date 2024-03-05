#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;

//使用额外的数组
class Solution {
public:
    void rotate(vector<int>& nums, int k) {
        vector<int> temp(nums.size());
        for(int j=0;j<nums.size();j++){
            int index=(j+k)%nums.size();
            temp[index]=nums[j];
        }
        nums.assign(temp.begin(),temp.end());
    }
};
//时间复杂度O(n)    空间复杂度O(n)

//环状替换
class Solution {
public:
    void rotate(vector<int>& nums, int k) {
       // k = k%nums.size();//有没有这句都不影响
        int count=gcd(k, nums.size());//最大公约数,遍历的次数(最小公倍数*最大公约数=两个数之积)
        for(int i=0;i<count;i++){
            int temp=nums[i];
            int current=i;
            do{
                int index=(current+k)%nums.size();
                swap(nums[index],temp);
                current=index;
            }while(current!=i);
        }

    }
};
//时间复杂度O(n)    空间复杂度O(1)

//翻转数组
//思路：先将所有元素翻转，这样尾部的k%n个元素就被移至数组头部，然后我们再翻转[0,k%n-1]区间的元素和[k%n,n-1]区间的元素即可
class Solution {
public:
    void rotate(vector<int>& nums, int k) {
        if(nums.size()>1){
            reverse(nums.begin(),nums.end());
            int index=k%nums.size()-1;
            reverse(nums.begin(),nums.begin()+index+1);
            index=k%nums.size();
            reverse(nums.begin()+index,nums.end());
        }
    }
};
//时间复杂度O(n)    空间复杂度O(1)