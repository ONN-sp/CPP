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

//递归法
class Solution {
private:
    TreeNode* pre = nullptr;//记录回溯过程中的上一个节点
    bool isBST(TreeNode* cur){
        if(cur==nullptr)
            return true;
        bool leftResult = isBST(cur->left);//左
        if(pre&&pre->val >= cur->val)//中  判断pre是为了处理第一个遍历的(最左)节点,因为此时它的前一个节点是空的
            return false;
        pre = cur;//记录前一个节点
        bool rightResult = isBST(cur->right);//右
        return leftResult&&rightResult;
    }
public:
    bool isValidBST(TreeNode* root) {
        if(root==nullptr)
            return true;
        return isBST(root);        
    }
};