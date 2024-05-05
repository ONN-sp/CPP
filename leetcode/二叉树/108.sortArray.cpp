#include <iostream>
#include <vector>
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

//递归法  不是直接在nums数组上操作,即不是传一半区间的数组去处理,而是用两个边界(left, right)来表示,类似二分法
class Solution {
private:
    TreeNode* sortArray(vector<int>& nums, int left, int right){
        if(left > right)
            return nullptr;
        int mid = (left+right)/2;
        TreeNode* node = new TreeNode(nums[mid]);
        node->left = sortArray(nums, left, mid-1);
        node->right = sortArray(nums, mid+1, right);
        return node;
    }
public:
    TreeNode* sortedArrayToBST(vector<int>& nums) {
        TreeNode* root = sortArray(nums, 0, nums.size()-1);
        return root;
    }
};

//迭代法  利用三个队列来模拟出递归二分构建节点的过程
class Solution {
public:
    TreeNode* sortedArrayToBST(vector<int>& nums) {  
        if(nums.size()==0)
            return nullptr;
        TreeNode* root = new TreeNode(0);//初始根节点
        queue<TreeNode*> nodeQue;//存放二叉搜索树节点
        queue<int> leftQue;//保存左区间下标
        queue<int> rightQue;//保存右区间下标
        nodeQue.push(root);
        leftQue.push(0);
        rightQue.push(nums.size()-1);
        while(!nodeQue.empty()){
            TreeNode* cur = nodeQue.front();
            nodeQue.pop();
            int left = leftQue.front();
            leftQue.pop();
            int right = rightQue.front();
            rightQue.pop();
            int mid = (left+right)/2;
            cur->val = nums[mid];
            if(left<=mid-1){//处理左区间
                cur->left = new TreeNode(0);
                nodeQue.push(cur->left);
                leftQue.push(left);
                rightQue.push(mid-1);
            }
            if(right>=mid+1){//处理右区间
                cur->right = new TreeNode(0);
                nodeQue.push(cur->right);
                leftQue.push(mid+1);
                rightQue.push(right);
            }
        }
        return root;
    }
};