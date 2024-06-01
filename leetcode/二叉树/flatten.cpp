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

//右-左-中
class Solution {
private:
    TreeNode* pre = nullptr;//记录前一个节点
    void dfs(TreeNode* cur){
        if(cur==nullptr)
            return;
        dfs(cur->right);//右
        dfs(cur->left);//左
        cur->right=pre;
        cur->left=nullptr;
        pre=cur;     
    }
public:
    void flatten(TreeNode* root) {
        dfs(root);
    }
};

//O(1)空间复杂度 寻找前驱节点(cur右子树的前驱节点)
class Solution {
public:
    void flatten(TreeNode* root) {
        TreeNode* pre_right = nullptr;
        TreeNode* cur = root;
        TreeNode* next = nullptr;
        while(cur!=nullptr){
            if(cur->left!=nullptr){
                next = cur->left;
                pre_right = next;
                while(pre_right->right!=nullptr)
                    pre_right = pre_right->right;
                pre_right->right = cur->right;
                cur->left =  nullptr;
                cur->right = next;
            }
            cur = cur->right;
        }
    }
};