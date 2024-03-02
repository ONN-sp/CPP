#include <iostream>
#include <vector>
using namespace std;

//暴力法
class Solution {
public:
    bool searchMatrix(vector<vector<int>>& matrix, int target) {
        for(auto& i:matrix){
            for(auto& j:i)
                if(j==target)
                    return true;
        }
        return false;

    }
};

//二分搜索
class Solution {
public:
    bool searchMatrix(vector<vector<int>>& matrix, int target) {
        for(auto& i:matrix){//每一行的数组元素
            //定义一个左闭右闭的二分区间,下面是左闭右闭的写法
            int left=0;//二分区间的左端点
            int right=matrix[0].size()-1;//二分区间的右端点
            while(left<=right){
                int middle=(left+right)/2;
                if(i[middle]>target)
                    right=middle-1;
                else if(i[middle]<target)
                    left=middle+1;
                else
                    return true;
            }
        }
        return false;
    }
};

//Z字搜索
class Solution {
public:
    bool searchMatrix(vector<vector<int>>& matrix, int target) {
        int col=matrix[0].size();
        int row=matrix.size();
        int i=0, j=col-1;
        while(i<=row-1 && j>=0){
                if(matrix[i][j]<target)
                    i++;
                else if(matrix[i][j]>target)
                    j--;
                else
                    return true;
            }
        return false;
        }
};