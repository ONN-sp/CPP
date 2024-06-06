#include <iostream>
#include <vector>
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
private:
    TreeNode* ConstructTree(vector<int>& preorder, int preorderBegin, int preorderEnd, vector<int>& inorder, int inorderBegin, int inorderEnd){
        //1. 如果数组大小=0,则说明是空节点
        if(preorderBegin==preorderEnd)//判断两个数组的任何一个都行
            return nullptr;
        //2. 如果不为空,则取先序数组第一个元素作为节点元素
        TreeNode* root = new TreeNode(preorder[preorderBegin]);//不能是0
        if(preorderEnd-preorderBegin==1)//说明此时分割后的左子树或右子树只有一个节点
            return root;
        //3. 找到先序数组第一个元素在中序数组的位置,作为切割点
        int delimiterIndex;
        for(delimiterIndex=inorderBegin;delimiterIndex<inorderEnd;delimiterIndex++)
            if(inorder[delimiterIndex]==preorder[preorderBegin])
                break;
        //4. 切割中序数组,切成中序左数组和中序右数组
        //采用左闭右开的区间
        int leftInorderBegin = inorderBegin;
        int leftInorderEnd = delimiterIndex;
        int rightInorderBegin = delimiterIndex+1;
        int rightInorderEnd = inorderEnd;
        //5. 切割先序数组,切成先序左数组和先序右数组
        //采用左闭右开的区间
        int leftPreorderBegin = preorderBegin+1;
        int leftPreorderEnd = preorderBegin+1+(leftInorderEnd-leftInorderBegin);
        int rightPreorderBegin = leftPreorderEnd;
        int rightPreorderEnd = preorderEnd;
        //6. 递归处理左区间和右区间
        root->left = ConstructTree(preorder, leftPreorderBegin, leftPreorderEnd, inorder, leftInorderBegin, leftInorderEnd);
        root->right = ConstructTree(preorder, rightPreorderBegin, rightPreorderEnd, inorder, rightInorderBegin, rightInorderEnd);
        return root;
    }
public:
    TreeNode* buildTree(vector<int>& preorder, vector<int>& inorder) {
        if(inorder.size()==0)
            return nullptr;
        return ConstructTree(preorder, 0, preorder.size(), inorder, 0, inorder.size());
    }
};


//从中序与后序遍历序列构造二叉树
class Solution {
private:
    TreeNode* ConstructTree(vector<int>& inorder, int inorderBegin, int inorderEnd, vector<int>& postorder, int postorderBegin, int postorderEnd){
        //1. 数组大小=0
        if(postorderBegin==postorderEnd)
            return nullptr;
        //2. 取后序数组最后一个元素为根节点
        TreeNode* root = new TreeNode(postorder[postorderEnd-1]);
        if(inorderEnd-inorderBegin==1)
            return root;
        //3. 中序数组中找切割点
        int delimiterIndex;
        for(delimiterIndex=inorderBegin; delimiterIndex < inorderEnd; delimiterIndex++)
            if(inorder[delimiterIndex]==root->val)
                break;
        //4. 切割中序数组
        int leftInorderBegin = inorderBegin;
        int leftInorderEnd = delimiterIndex;
        int rightInorderBegin = delimiterIndex+1;
        int rightInorderEnd = inorderEnd;
        //5. 切割后序数组
        int leftPostorderBegin = postorderBegin;
        int leftPostorderEnd = postorderBegin+(leftInorderEnd-leftInorderBegin);
        int rightPostorderBegin = leftPostorderEnd;
        int rightPostorderEnd = postorderEnd-1;
        //6. 递归左右子树
        root->left = ConstructTree(inorder, leftInorderBegin, leftInorderEnd, postorder, leftPostorderBegin, leftPostorderEnd);
        root->right = ConstructTree(inorder, rightInorderBegin, rightInorderEnd, postorder, rightPostorderBegin, rightPostorderEnd);
        return root;
    }
public:
    TreeNode* buildTree(vector<int>& inorder, vector<int>& postorder) {
        if(inorder.size()==0)
            return nullptr;
        return ConstructTree(inorder, 0, inorder.size(), postorder, 0, postorder.size());
    }
};