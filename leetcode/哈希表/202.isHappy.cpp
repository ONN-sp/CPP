#include <unordered_set>
using namespace std;

// sum值出现重复(且不为1)那么就不可能是快乐数
class Solution {
private:
    int getsum(int n){
        int sum = 0;
        while(n){
            sum += (n%10)*(n%10);
            n /=  10;
        }
        return sum;
    }
public:
    bool isHappy(int n) {
        unordered_set<int> ss;  
        int value = n;
        while(1){
            value = getsum(value);
            if(ss.find(value)!=ss.end())
                return false;
            ss.emplace(value);
            if(value==1)
                return true;
        }      
    }
};