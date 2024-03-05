//控制左右上下边界
#include<iostream>
#include<vector>
using namespace std;

class Solution {
public:
    vector<int> spiralOrder(vector<vector<int>>& matrix) {
      vector<int> result;
      int row=matrix.size();//行数
      int col=matrix[0].size();//列数
      int up=0,down=row-1,left=0,right=col-1;//上下左右边界
      while(1){
        for(int i=left;i<=right;i++)
            result.emplace_back(matrix[up][i]);
        if(++up>down)
            break;
        for(int i=up;i<=down;i++)
            result.emplace_back(matrix[i][right]);
        if(--right<left)
            break;
        for(int i=right;i>=left;i--)
            result.emplace_back(matrix[down][i]);
        if(--down<up)
            break;
        for(int i=down;i>=up;i--)
            result.emplace_back(matrix[i][left]);
        if(++left>right)
            break;
      }
      return result;
    }
};