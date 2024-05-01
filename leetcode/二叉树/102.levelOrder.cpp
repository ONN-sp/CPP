#include <iostream>
#include <queue>
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
    void layerOrder(TreeNode* cur, vector<vector<int>>& res, int depth){
        if(cur==nullptr)
            return;
        if(res.size()==depth)//！！！
            res.emplace_back(vector<int>());
        res[depth].emplace_back(cur->val);
        layerOrder(cur->left, res, depth+1);
        layerOrder(cur->right, res, depth+1);
    }    
public:
    vector<vector<int>> levelOrder(TreeNode* root) {
        int depth=0;//用来表示第几层
        vector<vector<int>> res;
        layerOrder(root, res, 0);
        return res;
    }
};

//迭代法
class Solution {
public:
    vector<vector<int>> levelOrder(TreeNode* root) {
        vector<vector<int>> res;
        queue<TreeNode*> qu;
        if(root==nullptr)
            return res;
        qu.push(root);
        while(!qu.empty()){
            int size = qu.size();
            vector<int> temp;
            while(size--){          
                TreeNode* cur = qu.front();
                temp.emplace_back(cur->val);
                qu.pop();
                if(cur->left)
                    qu.push(cur->left);
                if(cur->right)
                    qu.push(cur->right);
            }
            res.emplace_back(temp);
        } 
        return res;     
    }
};