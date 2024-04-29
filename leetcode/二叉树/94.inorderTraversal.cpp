#include <iostream>
#include <vector>
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
    void traversal(TreeNode* cur, vector<int>& res){
        if(cur==nullptr)
            return;
        traversal(cur->left, res);//左
        res.emplace_back(cur->val);//中
        traversal(cur->right, res);//右
    }
    vector<int> inorderTraversal(TreeNode* root) {
        vector<int> res;
        traversal(root, res);
        return res;
    }
};

//非递归法-迭代法
class Solution {
public:
    vector<int> inorderTraversal(TreeNode* root) {
        vector<int> res;
        stack<TreeNode*> st;
        if(root==nullptr)
            return res;
        TreeNode* cur = root;
        while(cur!=nullptr || !st.empty()){//两个条件缺一不可
            if(cur!=nullptr){//指针访问到非叶子节点
                st.push(cur);
                cur = cur->left;//左
            }
            else{//cur此时为空就从栈弹出top()  
                cur = st.top();//从栈里弹出的数据,就是要处理的数据(放进res)
                st.pop();
                res.emplace_back(cur->val);
                cur = cur->right;//右
            }     
        }
        return res;
    }
};