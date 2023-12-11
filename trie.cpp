#include "trie.h"
#include <algorithm>

Trie::Trie() : root(new TrieNode()) {}

Trie::~Trie()
{
    deleteNode(this->root);
    // 使用迭代释放内存（未完成）
    // for (std::vector<TrieNode*>::iterator it = this->nodes.begin(); it != this->nodes.end(); ++it) {
    //     delete *it;
    // }
    // this->nodes.clear();
}

// 递归删除节点
void Trie::deleteNode(TrieNode *node)
{
    if (node == NULL)
    {
        return;
    }
    std::unordered_map<char, TrieNode *> children = node->children;

    for (std::unordered_map<char, TrieNode *>::iterator it = children.begin(); it != children.end(); ++it)
    {
        deleteNode(it->second);
    }
    delete node;
    // for (std::unordered_map<char, TrieNode *>::iterator it = children.begin(); it != children.end(); ++it)
    // {
    //     delete it->second;
    // }
}

TrieNode *Trie::getRoot()
{
    return root;
}

void Trie::insert(const std::string &word)
{
    TrieNode *node = this->root;
    for (char ch : word)
    {
        if (node->children.find(ch) == node->children.end())
        {
            node->children[ch] = new TrieNode();
        }
        node = node->children[ch];
        // 新创建的节点放入nodes容器
        // this->nodes.push_back(node);
    }
    node->is_end_of_string = true;
}

std::vector<std::string> Trie::findStringWithCommonPrefix(const std::string &prefix)
{
    TrieNode *node = root;
    std::vector<std::string> result;

    for (char ch : prefix)
    {
        if (node->children.find(ch) == node->children.end())
        {
            return result;
        }
        node = node->children[ch];
    }

    getAllWordsFromNode(node, prefix, result);

    return result;
}

// DFS前缀树，记录最接近叶子节点的分支的前缀字符串
void Trie::traverseTrieDFS(
    TrieNode *node, std::string common_prefix, int current_branch_serial_number, 
    std::vector<std::string> &common_longest_prefix,
    std::unordered_set<int> &completed_branches)
{
    if (!node)
    {
        return;
    }
    node->traversed = true;
    // TODO:剪枝似乎还能优化
    for (auto &entry : node->children)
    {
        char ch = entry.first;
        if (completed_branches.find(current_branch_serial_number) != completed_branches.end())
        {
            traverseTrieDFS(entry.second, common_prefix + ch, current_branch_serial_number + 1, common_longest_prefix, completed_branches);
        }
        else
        {
            traverseTrieDFS(entry.second, common_prefix + ch, current_branch_serial_number, common_longest_prefix, completed_branches);
        }
    }

    if (node->children.size() > 1 && node->traversed && completed_branches.find(current_branch_serial_number) == completed_branches.end())
    {
        common_longest_prefix.push_back(common_prefix);
        completed_branches.insert(current_branch_serial_number);
    }
}

// // 用假数据测试一下
// int main()
// {
//     std::vector<std::string> thread_names = {
//         "HikariCP-myPool-1",
//         "HikariCP-myPool-2",
//         "Druid-ConnectionPool-Create-9",
//         "Druid-ConnectionPool-Create-3",
//         "pool-1-thread-1",
//         "pool-1-thread-2",
//         "pool-2-thread-1",
//         "pool-2-thread-2",
//         "rhg uaeiuh",
//     };
//     Trie *threads_trie = new Trie();
//     for (std::vector<std::string>::iterator it = thread_names.begin();
//                                          it != thread_names.end(); ++it)
//     {
//         threads_trie.insert(*it);
//     }
//     std::vector<std::string> common_longest_prefix;
//     std::unordered_set<int> completed_branches;

//     int current_branch_serial_number = 1;
//     threads_trie->traverseTrieDFS(threads_trie->getRoot(), "", current_branch_serial_number,
//                                   common_longest_prefix, completed_branches);

//     // Create a map to store the result
//     std::map<std::string, std::vector<std::string>> result;
//     std::string groupPrefix = "Group";
//     int groupSerialNumber = 0;
//     std::unordered_set<std::string> common_prefix_all_strings_set;

//     for (const std::string &prefix : common_longest_prefix)
//     {

//         std::vector<std::string> common_prefix_strings = threads_trie->findStringWithCommonPrefix(prefix);
//         // // 测试输出
//         // for (const std::string& item : thread_names) {
//         //     std::cout << item << std::endl;
//         // }
//         // 这里可以多做一层过滤
//         if (prefix == "")
//         {
//             result["Others:"] = common_prefix_strings;
//             break;
//         }
//         // // 测试输出
//         // for (const auto& element : common_prefix_strings) {
//         //     std::cout << element << std::endl;
//         // }
//         // Remove duplicates
//         std::vector<std::string> need_to_be_removed_strings;
//         for (const std::string &s : common_prefix_strings)
//         {
//             if (common_prefix_all_strings_set.find(s) != common_prefix_all_strings_set.end())
//             {
//                 need_to_be_removed_strings.push_back(s);
//             }
//         }
//         for (const std::string &s : need_to_be_removed_strings)
//         {
//             common_prefix_strings.erase(std::remove(common_prefix_strings.begin(), common_prefix_strings.end(), s), common_prefix_strings.end());
//         }

//         // Add to the result map
//         if (common_prefix_strings.size() > 1)
//         {
//             result[groupPrefix + " " + std::to_string(groupSerialNumber++) + ":"] = common_prefix_strings;
//         }
//         else
//         {
//             result["Others:"] = common_prefix_strings;
//         }

//         common_prefix_all_strings_set.insert(common_prefix_strings.begin(), common_prefix_strings.end());
//     }

//     // Find "Others" and add to the result map
//     std::unordered_set<std::string> all_thread_names_set(thread_names.begin(), thread_names.end());
//     // 找出两个集合的差集
//     for (const auto &word : common_prefix_all_strings_set)
//     {
//         all_thread_names_set.erase(word);
//     }
//     // 将差集放入Others组
//     if (!all_thread_names_set.empty())
//     {
//         result["Others:"] = std::vector<std::string>(all_thread_names_set.begin(), all_thread_names_set.end());
//     }
//     delete threads_trie;
//     // // Print the result 这是C++17写法
//     // for (const auto& [groupName, group] : result) {
//     //    std::cout << groupName << std::endl;
//     //    for (const std::string& item : group) {
//     //        std::cout << "  " << item << std::endl;
//     //    }
//     //    std::cout << std::endl;
//     // }
//     // printf("----------------------------start------------------------------\n");
//     // for (std::map<std::string, std::vector<std::string>>::iterator it = result.begin();
//     //                                                                 it != result.end(); ++it) {
//     //     const std::string& groupName = it->first;
//     //     const std::vector<std::string>& group = it->second;

//     //     // std::cout << groupName << "    totals:" << group.size() << std::endl;
//     //     printf("%s    totals: %zu\n", groupName.c_str(), group.size());

//     //     for (std::vector<std::string>::const_iterator itItem = group.begin(); itItem != group.end(); ++itItem) {
//     //         const std::string& item = *itItem;
//     //         // std::cout << "    " << item << "    " << "state:" << getJavaThreadState(thread_states[item]) << std::endl;
//     //         printf("    %s    state: %s\n", item.c_str(), getJavaThreadState(thread_states[item]).c_str());

//     //     }
//     //     printf("\n");
//     //     // std::cout << std::endl;
//     // }
//     // printf("-----------------------------end-------------------------------\n");
//     while (true)
//     {
//         main();
//     }

//     return 0;
// }
