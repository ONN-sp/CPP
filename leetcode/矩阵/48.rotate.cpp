#include <iostream>
#include <vector>
using namespace std;

//在选择矩阵中,一个元素对应4个元素的相互交换
class Solution {
public:
    void rotate(vector<vector<int>>& matrix) {
        int n = matrix.size();//题目说了这是一个方阵
        for(int i=0;i<n/2;i++){//从外往内的第i圈
            for(int j=i;j<n-i-1;j++){//在第i圈的第一行上,表示这一行的第j个元素
                //使用一个临时变量来实现四个位置的元素的原地交换
                int temp=matrix[i][j];
                matrix[i][j]=matrix[n-j-1][i];
                matrix[n-j-1][i]=matrix[n-i-1][n-j-1];
                matrix[n-i-1][n-j-1]=matrix[j][n-i-1];
                matrix[j][n-i-1]=temp;
            }
        }
    }
};