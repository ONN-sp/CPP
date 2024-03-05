//哈希表+滑动窗口
#include<iostream>
#include<vector>
#include<unordered_map>                                                                                                                                                   
using namespace std;

class Solution {
public:
    string minWindow(string s, string t) {
        unordered_map<char, int> hash;
        string result;
        if(s.size()<t.size())
            return result;
        for(int i=0;i<t.size();i++)//哈希表处理频次
            hash[t[i]]++;
        int slow=0;
        int fast=0;
        int count_num=0;
        while(slow<=fast&&fast<s.size()){
            while(fast<s.size()&&count_num<t.size()){
                //右指针移动,寻找包含t字符串的较大窗口
                if(hash.count(s[fast])){
                    if(hash[s[fast]]>0)
                        count_num++;
                    hash[s[fast]]--;
                }
            fast++;
            }
            if(count_num==t.size()){
                //左指针移动,在已经得到的满足区间内找此区间中最小包含t字符串的区间
                while(!hash.count(s[slow])||(hash.count(s[slow])&&hash[s[slow]]<0)){
                    if(hash.count(s[slow])&&hash[s[slow]]<0)
                        hash[s[slow]]++;
                    slow++;
                    }
                if(result.size()==0||result.size()>fast-slow)
                    result = s.substr(slow, fast-slow);
                if(fast-slow==t.size())//因为题目说了结果唯一,所以当找到了一个长度和t一样的子串且包含了t字符串,那么他一定是最小的
                    return result;
                hash[s[slow]]++;//slow指针指向的位置一定是t字符串中有的字符,因为到此时的slow后左指针就不能移动了
                slow++;
                count_num--;
            }
        }
        return result;
    }
};