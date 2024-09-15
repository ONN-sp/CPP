#include <iostream>
#include <vector>
#include <unordered_map>
using namespace std;

struct TreeNode {
    int val;
    TreeNode *left;
    TreeNode *right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};

//路径总和Ⅰ
class Solution {
private:
    bool traversal(TreeNode* cur, int target){
        if(cur->left==nullptr&&cur->right==nullptr&&cur->val==target)        //访问到叶子节点 终止
            return true; 
        if(cur->left==nullptr&&cur->right==nullptr)
            return false; 
        if(cur->left)    
            if(traversal(cur->left, target-cur->val))
                    return true;
        if(cur->right)
            if(traversal(cur->right, target-cur->val))
                    return true;
        return false;
    }
public:
    bool hasPathSum(TreeNode* root, int targetSum) {
        if(root==nullptr)
            return false;      
        return traversal(root, targetSum);
        
    }
};

//路径总和Ⅱ
class Solution {
private:
    vector<int> path;//存每一条合适的路径
    vector<vector<int>> res;//存所有路径
    void traversal(TreeNode* cur, int target){
        if(cur->left==nullptr&&cur->right==nullptr&&cur->val==target){
            res.emplace_back(path);
            return;
        }
        if(cur->left==nullptr&&cur->right==nullptr)
            return;
        if(cur->left){
            path.emplace_back(cur->left->val);
            traversal(cur->left, target-cur->val);
            path.pop_back();
        }
        if(cur->right){
            path.emplace_back(cur->right->val);
            traversal(cur->right, target-cur->val);
            path.pop_back();
        }
    }
public:
    vector<vector<int>> pathSum(TreeNode* root, int targetSum) {    
        if(root==nullptr)
            return res;
        path.emplace_back(root->val);
        traversal(root, targetSum);
        return res;
    }
};

//路径总和Ⅲ
//前缀和+哈希表
class Solution {
private:
    unordered_map<long, int> mm;//key:前缀和,value:向下递推时此时路径中前缀和=key出现的次数
    int dfs(TreeNode* cur, long curr, int targetSum){
        if(cur==nullptr)
            return 0;
        curr += cur->val;     
        int ret = 0;
        if(mm.count(curr-targetSum))//在前缀和中查找是否有符合条件的
           ret = mm[curr-targetSum];//不能是ret=1,因为在向下递推的路径中可能出现多次满足条件的前缀和
        mm[curr]++;//必须放在if(mm.count(curr-targetSum))后,不然对于targetSum=0的情况就会出错
        ret += dfs(cur->left, curr, targetSum);
        ret += dfs(cur->right, curr, targetSum);
        mm[curr]--;//为了保证找到的前缀和都是从父节点到子节点,必须在这--
        return ret;
    }
public:
    int pathSum(TreeNode* root, int targetSum) {
        if(root==nullptr)
            return 0;
        mm[0] = 1;//初始前缀和一定要有,不然就会丢掉从根节点root出发满足条件的前缀和
        return dfs(root, 0, targetSum);
    }
};