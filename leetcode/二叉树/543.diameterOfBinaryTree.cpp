#include <iostream>
using namespace std;

struct TreeNode {
    int val;
    TreeNode *left;
    TreeNode *right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};

//递归法
class Solution {
private:
    int result=0;
    //求当前节点的深度
    int depth(TreeNode* cur){
        if(cur==nullptr)
            return 0;
        int leftDepth = depth(cur->left);
        int rightDepth = depth(cur->right);
        int result_temp = leftDepth+rightDepth+1;
        result = max(result_temp, result);
        int com_result = max(leftDepth, rightDepth) + 1;
        return com_result;
    }
public:
    int diameterOfBinaryTree(TreeNode* root) {
        if(root==nullptr)
            return 0;
        depth(root);
        return result - 1;
    }
};