#include <iostream>
using namespace std;

struct TreeNode {
    TreeNode* left;
    TreeNode* right;
    int val;
    TreeNode():left(nullptr),right(nullptr),val(0){}
    TreeNode(int x):left(nullptr),right(nullptr),val(x){}
    TreeNode(TreeNode* left, TreeNode* right, int x):left(left),right(right),val(x){}
};

//左-右-中
//解题关键:1.节点的最大贡献值;2.节点最大贡献值与该节点的最大路径和的关系
class Solution {
private:
    int maxSum = INT_MIN;
    int traversal(TreeNode* cur){//返回的是最大贡献值
        if(cur==nullptr)
            return 0;   
        //递归计算左右子节点的最大贡献值
        //只有在最大贡献值>0时,才会选取对应子节点
        int left = max(traversal(cur->left), 0);
        int right = max(traversal(cur->right), 0);
        // 节点的最大路径和取决于该节点的值与该节点的左右子节点的最大贡献值
        int res = cur->val + max(left, right);//当前节点cur的最大贡献值
        maxSum =  max(cur->val + left + right, maxSum);
        return res;
    }
public:
    int maxPathSum(TreeNode* root) {  
        if(root==nullptr)
            return 0; 
        traversal(root);
        return maxSum;
    }
};