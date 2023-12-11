#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>

class TrieNode
{
public:
    std::unordered_map<char, TrieNode *> children;
    bool is_end_of_string;
    bool traversed;

    TrieNode() : is_end_of_string(false), traversed(false) {}
    // // 似乎不需要这个析构
    // ~TrieNode()
    // {
    //     for (std::unordered_map<char, TrieNode *>::iterator it = children.begin(); it != children.end(); ++it)
    //     {
    //         delete it->second;
    //     }
    // }
};

class Trie
{
private:
    TrieNode *root;

    // 记录节点，提高析构速度
    std::vector<TrieNode *> nodes;

    void getAllWordsFromNode(TrieNode *node, const std::string &currentPrefix, std::vector<std::string> &result)
    {
        if (node->is_end_of_string)
        {
            result.push_back(currentPrefix);
        }

        for (auto &entry : node->children)
        {
            TrieNode *childNode = entry.second;
            getAllWordsFromNode(childNode, currentPrefix + entry.first, result);
        }
    }

public:
    Trie();

    ~Trie();

    TrieNode *getRoot();

    void insert(const std::string &word);

    void deleteNode(TrieNode *node);

    std::vector<std::string> findStringWithCommonPrefix(const std::string &prefix);
    // DFS前缀树，记录最接近叶子节点的分支的字符串
    void traverseTrieDFS(
        TrieNode *node, std::string commonPrefix, int currentBranchSerialNumber,
        std::vector<std::string> &commonLongestPrefix,
        std::unordered_set<int> &completedBranches);
};