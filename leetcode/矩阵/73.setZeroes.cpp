#include<iostream>
#include<vector>
using namespace std;

//两个标记数组
class Solution {
public:
    void setZeroes(vector<vector<int>>& matrix) {
        vector<int> rows(matrix.size());//记录矩阵中0元素的行标,初始都为0
        vector<int> columns(matrix[0].size());//记录矩阵中0元素的列标,初始都为0
        for(int i=0;i<matrix.size();i++){
            for(int j=0;j<matrix[0].size();j++){
                if(matrix[i][j]==0){
                    rows[i]=1;
                    columns[j]=1;
                }
            }
        }
        for(int i=0;i<matrix.size();i++){
            for(int j=0;j<matrix[0].size();j++){
                if(rows[i]||columns[j]){
                    matrix[i][j]=0;
                    }
                }
            }
    }
};
//时间复杂度:O(mn)  空间复杂度:O(m+n)

//两个标记变量
class Solution {
public:
    void setZeroes(vector<vector<int>>& matrix) {
        int row=0, col=0;//两个标记变量,记录第一行和第一列是否有0
        for(int i=0;i<matrix[0].size();i++)//检查第一行是否有0
            if(matrix[0][i]==0)
               row=1;
        for(int i=0;i<matrix.size();i++)//检查第一列是否有0
            if(matrix[i][0]==0)
               col=1; 
        for(int i=1;i<matrix.size();i++){//检查后续行列,若有0则在第一行第一列对应下标处设为0
            for(int j=1;j<matrix[0].size();j++){
                if(matrix[i][j]==0){
                    matrix[i][0]=0;//第一列保存为0的行标,用0表示后续列中有0
                    matrix[0][j]=0;//第一行保存为0的列标,用0表示后续行中有0
                }
            }
        }
        for(int i=1;i<matrix.size();i++){//按照第一行第一列的0情况给后续行列置零
           for(int j=1;j<matrix[0].size();j++){
               if(!matrix[i][0]||!matrix[0][j])
                matrix[i][j]=0;
           } 
        }
        if(row){
            for(int i=0;i<matrix[0].size();i++)//按照row标记给第一行置零
                matrix[0][i]=0;
        }
        if(col){
            for(int j=0;j<matrix.size();j++)//按照col标记给第一列置零
                matrix[j][0]=0;
        }
    }
};
//时间复杂度:O(mn)  空间复杂度:O(1)

//一个标记变量
class Solution {
public:
    void setZeroes(vector<vector<int>>& matrix) {
        int flag=0;//记录第一行是否有0
        for(int i=0;i<matrix[0].size();i++)
            if(matrix[0][i]==0)
                flag=1;
        cout << flag << endl;
        for(int j=0;j<matrix.size();j++)
            if(matrix[j][0]==0)
                matrix[0][0]=0;//记录第一列是否有0
        cout << matrix[0][0] << endl;
        for(int i=1;i<matrix.size();i++){
            for(int j=1;j<matrix[0].size();j++){
                if(matrix[i][j]==0){
                    matrix[i][0]=0;
                    matrix[0][j]=0;
                }
            }
        }
        for(int i=1;i<matrix.size();i++){
            for(int j=1;j<matrix[0].size();j++){
                if(!matrix[i][0]||!matrix[0][j])
                    matrix[i][j]=0;
            }
        }
        if(!matrix[0][0]){
            for(int i=0;i<matrix.size();i++)
                matrix[i][0]=0;
        }    
        if(flag){
            for(int i=0;i<matrix[0].size();i++)
                matrix[0][i]=0;
        }  
    }
};
//时间复杂度:O(mn)  空间复杂度:O(1)