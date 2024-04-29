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

class Solution {
public:
    TreeNode* flipTree(TreeNode* cur){
        if(cur==nullptr)
            return nullptr;
        flipTree(cur->left);//左
        flipTree(cur->right);//右 
        /*
        TreeNode* temp;
        temp=cur->left;
        cur->left=cur->right;
        cur->right=temp;
        */
       swap(cur->left, cur->right);//中 
        return cur;
    }
    TreeNode* invertTree(TreeNode* root) {
        TreeNode* res;
        res=flipTree(root);
        return res;
    }
};