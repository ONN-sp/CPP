#include <vector>

using namespace std;

class Solution {
private:
    int rows, cols;
    bool dfs(vector<vector<char>>& board, string word, int i, int j, int k){//i:行数;j:列数;k:待匹配的字符下标
        if(i>=rows || i<0 ||j>=cols || j<0 || board[i][j]!=word[k])//索引越界+与目标字符不匹配
            return false;
        if(k==word.size()-1)//字符串word被全部匹配成功
            return true;
        board[i][j] = '\0';//标记空字符:因为同一个单元格内的字母在递推过程中不允许重复使用,所以在向下递推过程中对于board中已经被检查了的字符要标记
        bool res = dfs(board, word, i+1, j, k+1) || dfs(board, word, i-1, j, k+1) || dfs(board, word, i, j-1, k+1) || dfs(board, word, i, j+1, k+1);//下 上 左 右
        board[i][j] = word[k];//回溯过程必须把board[i][j]还原,不然下一个单元格字符开始匹配时,它的上、左两个位置的字符就都是'\0',即这两个位置单元格本来是正确匹配的都不行了
        return res;
    }
public:
    bool exist(vector<vector<char>>& board, string word) {
        rows = board.size();//行数
        cols = board[0].size();//列数
        for(int i=0;i<rows;i++){
            for(int j=0;j<cols;j++){
                if(dfs(board, word, i, j, 0))
                    return true;
            }
        }
        return false;
    }
};