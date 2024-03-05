#include<iostream>
#include<vector>
#include<unordered_map>
#include<algorithm>
using namespace std;

//排序法
class Solution {
public:
    vector<vector<string>> groupAnagrams(vector<string>& strs) {
        vector<vector<string>> ss;
        unordered_map<string, vector<string>> mm;
        for(int i = 0; i<strs.size(); i++){
            string key;
            vector<string> keys;
            key = strs[i];
            sort(key.begin(), key.end());
            mm[key].emplace_back(strs[i]);//像unordered_map容器插入键值对时,遇到相同键值它就会自动把对应的所有value合在一个,然后对应这个key
        }
        for(auto s = mm.begin(); s!=mm.end(); s++){
            ss.emplace_back(s->second);
        }
        return ss;
    }
};

//计数法
class Solution {
public:
    vector<vector<string>> groupAnagrams(vector<string>& strs) {
        unordered_map<string, vector<string>> mm;
        vector<vector<string>> result;
            for(auto& str:strs){
                string times(26, '0');
                for(auto& cc:str){
                    times[cc-'a']+=1;//'b'-'a'=1,'0'+1='1'
                }
                mm[times].emplace_back(str);
            }
            for(auto& c:mm){
                result.emplace_back(c.second);//c不是某个键值对的迭代器,所以直接.进行索引而不是->
            }
            return result;
    }
};