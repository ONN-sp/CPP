#include<iostream>
#include<vector>
using namespace std;

//滑动窗口法1,时间复杂度 O(m+(n−m)×Σ)
class Solution {
public:
    vector<int> findAnagrams(string s, string p) {
        vector<int> ss(26), pp(26);
        vector<int> result;
        if(s.size()<p.size())
            return result;
        for(int i=0;i<p.size();i++){
            //初始化第一个窗口
            ss[s[i]-'a']++;
            pp[p[i]-'a']++;
        }
        if(ss==pp)
            result.emplace_back(0);
        for(int i=0;i<s.size()-p.size();i++){
            //下面这两行太牛了,此方法没有用左右指针,即没有考虑左指针的剪枝,而是依序一个一个移动判断ss==pp
            ss[s[i]-'a']--;
            ss[s[i+p.size()]-'a']++;
            if(ss==pp)
                result.emplace_back(i+1);
        }
        return result;
    }
};

//滑动窗口法2
//优化算法:不去比较滑动窗口的字符串和p字符串,而是统计这两个字符串的不同字母的个数differ,然后通过判断differ是否为0来得结论
class Solution {
public:
    vector<int> findAnagrams(string s, string p) {
        vector<int> result;
        vector<int> count(26);
        if(s.size()<p.size())
            return result;
        for(int i=0;i<p.size();i++){
            //先判断前面p.size()个字符,即先构建第一个窗口
            count[p[i]-'a']--;
            count[s[i]-'a']++;
        }
        int differ=0;
        for(int i=0;i<26;i++){
            if(count[i]!=0)
                differ++;
        }
        if(differ==0)
            result.emplace_back(0);
        for(int i=0; i<s.size()-p.size();i++){
            //此时的窗口为[i+1,...,i+p.size()],如新建一个窗口时是去掉s[i]添加s[i+p.size()]
            if(count[s[i]-'a']==1)
                differ--;//窗口去掉一个p字符串中没有的字符,所以给differ--
            else if(count[s[i]-'a']==0)
                differ++;//窗口去掉一个p字符串有的字符,则给differ++
            count[s[i]-'a']--;//把count[s[i]-'a']的取值设为-1或0
            if(count[s[i+p.size()]-'a']==-1)
                differ--;//此时新进入窗口的字符是刚刚窗口去掉的字符,且这个字符在第一个窗口中匹配到了p字符串中的字符
            else if(count[s[i+p.size()]-'a']==0)
                differ++;//此时新进入窗口的字符不是刚刚去掉的字符,且这个字符在第一个窗口中没有匹配到p字符串中的字符
            count[s[i+p.size()]-'a']++;
            if(differ==0)
                result.emplace_back(i+1);
        }
        return result;
    }
};