#include <iostream>
#include <stack>
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
public:
    bool compare(TreeNode* leftNode, TreeNode* rightNode){
        if(leftNode==nullptr&&rightNode==nullptr)
            return true;
        else if(leftNode==nullptr||rightNode==nullptr)//一个子树为空,另一个不为空
            return false;
        else if(leftNode->val==rightNode->val){
            bool outside = compare(leftNode->left, rightNode->right);//外侧.    左子树的"左";右子树的"右"
            bool inside = compare(leftNode->right, rightNode->left);//内侧.   左子树的"右";右子树的"左"  
            bool result = outside&&inside;//逻辑处理过程   左子树"中";右子树"中" 
            return result;    
        }
        else
            return false;
    }
    bool isSymmetric(TreeNode* root) {
        if(root==nullptr)
            return true;
        return compare(root->left, root->right);
    }
};

//迭代法
class Solution {
public:
    bool isSymmetric(TreeNode* root) {
        stack<TreeNode*> st;
        if(root==nullptr)
            return true;
        st.push(root->left);
        st.push(root->right);
        while(!st.empty()){
            TreeNode* curLeft = st.top();
            st.pop();
            TreeNode* curRight = st.top();
            st.pop();
            if(curLeft==nullptr&&curRight==nullptr)
                continue;
            else if(curLeft==nullptr||curRight==nullptr)
                return false;
            else if(curLeft->val!=curRight->val)
                return false;  
            //内侧
            st.push(curLeft->right);
            st.push(curRight->left);
            //外侧
            st.push(curLeft->left);
            st.push(curRight->right);
        }
        return true;
    }
};