#include<iostream>
#include<vector>
#include<unordered_set>
#include<algorithm>
using namespace std;

class Solution {
public:
    int longestConsecutive(vector<int>& nums) {
        unordered_set<int> set;
        vector<int> num_temp;
        for(const int& num:nums){
            num_temp.emplace_back(num);
            set.insert(num);
        }
        int size = num_temp.size();
        int longestStreak = 0;
        sort(num_temp.begin(), num_temp.end());
        set.clear();//清空
        for(int i=0;i<size;i++){
            set.emplace(num_temp[i]);   
        }
        //set容器排序完成
        for(const int& num:set){
            // cout << num-1 << endl;
            if(!set.count(num+1)){
                //set容器中有num
                int currentStreak = 1;
                int currentnum = num;
                while(set.count(currentnum-1)){
                    currentStreak++;
                    currentnum--; 
                }
                longestStreak = max(longestStreak ,currentStreak);//比较连续序列长,并取最大的返回
            }
        }
        return longestStreak;
    }
};