//排序+合并
#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;

class Solution {
public:
    vector<vector<int>> merge(vector<vector<int>>& intervals) {
        vector<vector<int>> result;
        sort(intervals.begin(),intervals.end());//按照区间左端点进行排序的.第i-1个区间和第i个区间如果不重叠,那么它一定也不会和i后面的区间重叠
        result.emplace_back(intervals[0]);
        for(int i=1;i<intervals.size();i++){
            if(result.back()[1]>=intervals[i][0])//判断重
                result.back()[1]=max(result.back()[1],intervals[i][1]);//合并操作
            else//此时不重叠了,则产生了一个合并好的区间,那么导入后一个intervals区间再进行相同的重叠判断
                result.emplace_back(intervals[i]);
        }
        return result;
    }
};