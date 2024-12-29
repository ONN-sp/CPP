// 深搜  会超时
#include <iostream>
#include <vector>
#include <unordered_set>

using namespace std;

vector<string> path;
vector<vector<string>> res;
bool isOneCharDifferent(const string& str1, const string& str2) {
    if (str1.length() != str2.length()) {
        return false;
    }
    int diffCount = 0;
    for (size_t i = 0; i < str1.length(); ++i) {
        if (str1[i] != str2[i]) {
            diffCount++;
            if (diffCount > 1) {
                return false;
            }
        }
    }
    return diffCount == 1;
}
void dfs(const string& beginWord, const string& endWord, vector<string>& wordList, unordered_set<string>& visited) {
    if (beginWord == endWord) {
        res.push_back(path);
        // for (const auto& c : path) {
        //     cout << c << endl;
        // }
        return;
    }
    for (auto it = wordList.begin(); it != wordList.end(); ++it) {
        if (isOneCharDifferent(beginWord, *it) && visited.find(*it) == visited.end()) {
            path.push_back(*it);
            visited.insert(*it);
            dfs(*it, endWord, wordList, visited);
            path.pop_back();
            visited.erase(*it);
        }
    }
}
int ladderLength(string beginWord, string endWord, vector<string>& wordList) {
    unordered_set<string> visited;
    visited.insert(beginWord);
    dfs(beginWord, endWord, wordList, visited);
    if(res.empty())
        return 0;
    int result = res[0].size();
    for (const auto& p : res) {
        result = min(result, (int)p.size());
    }
    return result+1; // 加1是因为路径长度包括开始单词
}

int main() {
    string beginWord = "hit";
    // string beginWord = "qa";
    // string endWord = "sq";
    string endWord = "cog";
    // vector<string> wordList = {"hot","dot","dog","lot","log","cog"};
    // vector<string> wordList = {"hot","dot","dog","lot","log"};
    vector<string> wordList = {"si","go","se","cm","so","ph","mt","db","mb","sb","kr","ln","tm","le","av","sm","ar","ci","ca","br","ti","ba","to","ra","fa","yo","ow","sn","ya","cr","po","fe","ho","ma","re","or","rn","au","ur","rh","sr","tc","lt","lo","as","fr","nb","yb","if","pb","ge","th","pm","rb","sh","co","ga","li","ha","hz","no","bi","di","hi","qa","pi","os","uh","wm","an","me","mo","na","la","st","er","sc","ne","mn","mi","am","ex","pt","io","be","fm","ta","tb","ni","mr","pa","he","lr","sq","ye"};
    int res = ladderLength(beginWord, endWord, wordList);
    cout << res << endl;
    return 0;
}

// 广搜
// class Solution {
// private:
//     int bfs(string beginWord, string endWord, vector<string>& wordList) {
//         unordered_set<string> wordSet(wordList.begin(), wordList.end());// 将vector转换成unordered_set,提高查询速度
//         unordered_map<string, int> visitMap;// <word, 查询到这个word时的路径长度>
//         visitMap[beginWord] = 1;
//         queue<string> que;
//         que.push(beginWord);
//         while(!que.empty()) {
//             string word = que.front();
//             que.pop();
//             int pathLength = visitMap[word];
//             for(int i=0;i<word.size();i++) {
//                 string newWord = word;
//                 for(int j=0;j<26;j++) {
//                     newWord[i] = j+'a';
//                     if(newWord==endWord&&wordSet.find(newWord)!=wordSet.end())// 通过一个字符一个字符的修改找到了endWord且这个字符串位于wordList中
//                         return pathLength+1;
//                     if(wordSet.find(newWord)!=wordSet.end() && visitMap.find(newWord)==visitMap.end()) {
//                         visitMap.emplace(pair<string, int>(newWord, pathLength+1));
//                         que.push(newWord);
//                     }
//                 }
//             }
//         }
//         return 0;
//     }
// public:
//     int ladderLength(string beginWord, string endWord, vector<string>& wordList) {
//         return bfs(beginWord, endWord, wordList);
//     }
// };