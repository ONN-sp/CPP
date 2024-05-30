#include <iostream>
#include <vector>
using namespace std;

struct TreeNode{
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int val): val(val), left(nullptr), right(nullptr){}
    TreeNode(TreeNode* left, TreeNode* right): val(0), left(left), right(right){}
    TreeNode(): val(0), left(nullptr), right(nullptr){}
};

//广搜
class Solution {
private:
    void layerOrder(TreeNode* cur, vector<vector<int>>& resultVal, int depth){
        if(cur==nullptr)
            return;
        if(resultVal.size()==depth)
            resultVal.emplace_back(vector<int>());
        resultVal[depth].emplace_back(cur->val);
        layerOrder(cur->left, resultVal, depth+1);
        layerOrder(cur->right, resultVal, depth+1);
    }
public:
    vector<int> rightSideView(TreeNode* root) {
        vector<int> res;
        if(root==nullptr)
            return res;
        vector<vector<int>> resultVal;
        layerOrder(root, resultVal, 0);
        // for(auto& c:resultVal)
        //     cout << c[c.size()-1] << endl;
        for(auto& elem:resultVal)
            res.emplace_back(elem[elem.size()-1]);
        return res;
    }
};

//深搜  根-右-左
class Solution {
private:
    void dfs(TreeNode* cur, vector<int>& res, int depth){
        if(cur==nullptr)
            return;
        if(res.size()==depth)
            res.emplace_back(cur->val);
        dfs(cur->right, res, depth+1);
        dfs(cur->left, res, depth+1);       
    }
public:
    vector<int> rightSideView(TreeNode* root) {
        vector<int> res;      
        if(root==nullptr)
            return res;
        dfs(root, res, 0);
        return res;
    }
};