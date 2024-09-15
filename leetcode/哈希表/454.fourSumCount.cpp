#include <vector>
#include <unordered_map>

using namespace std;

// 此题可以用哈希表法(和四数求和不同),因为不涉及去重啥的
class Solution {
public:
    int fourSumCount(vector<int>& nums1, vector<int>& nums2, vector<int>& nums3, vector<int>& nums4) {
        int res = 0;
        unordered_map<int, int> mm;// key:nums1+nums2的数  value:nums1+nums2出现的次数
        for(int i:nums1){
            for(int j:nums2)
                mm[i+j]++;
        }
        for(int k:nums3){
            for(int p:nums4){
                if(mm.find(0-k-p)!=mm.end())
                    res += mm[0-k-p];
            }
        }
        return res;
    }
};