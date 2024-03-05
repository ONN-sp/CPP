#include<iostream>
#include<vector>
#include<unordered_map>
using namespace std;

//滑动窗口不能向双指针那样可以根据题意指针跳着访问,滑动窗口的右指针肯定是要一个一个遍历的,而左指针可以跳着访问
//滑动窗口法关注的是两个指针间的区间(内容、长度等),双指针关注的两个指针端点
//滑动窗口如何更新左指针很重要
class Solution {
public:
    int lengthOfLongestSubstring(string s) {
        unordered_map<char, int> mm;
        int first=-1;
        int result=0;
        for(int second=0;second<s.size();second++){
            if(mm.count(s[second])){
                first=max(first, mm[s[second]]);
            }
            mm[s[second]]=second;
            result=max(result,second-first);  
        }
        return result;
    }
};