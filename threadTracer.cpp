/**
 * 功能目前完成
 * TODO:
 *      剪枝（这个太伤脑了，看什么时候状态好的时候想一想）
*/

#include <jvmti.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <mutex>
#include "trie.cpp"

// static jrawMonitorID vmtrace_lock;
// static jlong start_time;
static jvmtiEnv* jvmti;
std::mutex mtx;  // 全局互斥锁

std::string getJavaThreadState (jint state) {
    switch (state & JVMTI_JAVA_LANG_THREAD_STATE_MASK) {
        case JVMTI_JAVA_LANG_THREAD_STATE_NEW:
            return "NEW";
        case JVMTI_JAVA_LANG_THREAD_STATE_TERMINATED:
            return "TERMINATED";
        case JVMTI_JAVA_LANG_THREAD_STATE_RUNNABLE:
            return "RUNNABLE";
        case JVMTI_JAVA_LANG_THREAD_STATE_BLOCKED:
            return "BLOCKED";
        case JVMTI_JAVA_LANG_THREAD_STATE_WAITING:
            return "WAITING";
        case JVMTI_JAVA_LANG_THREAD_STATE_TIMED_WAITING:
            return "TIMED_WAITING";
        default:
            return "UNKNOWN";
    }
}

void groupbyThread(
            std::unordered_map<std::string, jint>& thread_states,
            Trie* threads_trie, std::vector<std::string>& thread_names,
            std::vector<std::string>& common_longest_prefix, 
            std::unordered_set<int>& completed_branches) {
    // Sort by string length in descending order
    // 是否保留排序操作有待考量
    std::sort(thread_names.begin(), thread_names.end(), [](const std::string& a, const std::string& b) {
        return a.length() > b.length();
        });

    int current_branch_serial_number = 1;

    threads_trie->traverseTrieDFS(threads_trie->getRoot(), "", current_branch_serial_number, 
                                            common_longest_prefix, completed_branches);

    // Create a map to store the result
    std::map<std::string, std::vector<std::string>> result;
    std::string groupPrefix = "Group";
    int groupSerialNumber = 0;
    std::unordered_set<std::string> common_prefix_all_strings_set;

    for (const std::string& prefix : common_longest_prefix) {

        std::vector<std::string> common_prefix_strings = threads_trie->findStringWithCommonPrefix(prefix);
        // // 测试输出
        // for (const std::string& item : thread_names) {
        //     std::cout << item << std::endl;
        // }
        // 这里可以多做一层过滤
        if (prefix == "") {
            result["Others:"] = common_prefix_strings;
            break;  
        }
        // // 测试输出
        // for (const auto& element : common_prefix_strings) {
        //     std::cout << element << std::endl;
        // }
        // Remove duplicates
        std::vector<std::string> need_to_be_removed_strings;
        for (const std::string& s : common_prefix_strings) {
            if (common_prefix_all_strings_set.find(s) != common_prefix_all_strings_set.end()) {
                need_to_be_removed_strings.push_back(s);
            }
        }
        for (const std::string& s : need_to_be_removed_strings) {
            common_prefix_strings.erase(std::remove(common_prefix_strings.begin(), common_prefix_strings.end(), s), common_prefix_strings.end());
        }

        // Add to the result map
        if (common_prefix_strings.size() > 1) {
            result[groupPrefix + " " + std::to_string(groupSerialNumber++) + ":"] = common_prefix_strings;
        }
        else {
            result["Others:"] = common_prefix_strings;
        }

        common_prefix_all_strings_set.insert(common_prefix_strings.begin(), common_prefix_strings.end());
    }

    // Find "Others" and add to the result map
    std::unordered_set<std::string> all_thread_names_set(thread_names.begin(), thread_names.end());
    // 找出两个集合的差集
    for (const auto& word : common_prefix_all_strings_set) {
        all_thread_names_set.erase(word);
    }
    // 将差集放入Others组
    if (!all_thread_names_set.empty()) {
        result["Others:"] = std::vector<std::string>(all_thread_names_set.begin(), all_thread_names_set.end());
    }
    // // Print the result 这是C++17写法
    // for (const auto& [groupName, group] : result) {
    //    std::cout << groupName << std::endl;
    //    for (const std::string& item : group) {
    //        std::cout << "  " << item << std::endl;
    //    }
    //    std::cout << std::endl;
    // }
    printf("----------------------------start------------------------------\n");
    for (std::map<std::string, std::vector<std::string>>::iterator it = result.begin(); 
                                                                    it != result.end(); ++it) {
        const std::string& groupName = it->first;
        const std::vector<std::string>& group = it->second;

        // std::cout << groupName << "    totals:" << group.size() << std::endl;
        printf("%s    totals: %zu\n", groupName.c_str(), group.size());

        for (std::vector<std::string>::const_iterator itItem = group.begin(); itItem != group.end(); ++itItem) {
            const std::string& item = *itItem;
            // std::cout << "    " << item << "    " << "state:" << getJavaThreadState(thread_states[item]) << std::endl;
            printf("    %s    state: %s\n", item.c_str(), getJavaThreadState(thread_states[item]).c_str());

        }
        printf("\n");
        // std::cout << std::endl;
    }
    printf("-----------------------------end-------------------------------\n");
}

void statisticThreads() {
    // 构造Trie类型的threads_trie和std::vector<std::string>的thread_names
    Trie* threads_trie = new Trie();
    std::vector<std::string> thread_names;
    std::unordered_set<int> completed_branches;
    std::vector<std::string> common_longest_prefix;
    // std::unordered_map<std::string, jvmtiThreadInfo> thread_infos;
    std::unordered_map<std::string, jint> thread_states;

    jint threads_count;
    jthread* threads;
    // 获取所有线程
    jvmtiError error = (jvmti)->GetAllThreads(&threads_count, &threads);
    if (error != JVMTI_ERROR_NONE) {
        std::cerr << "Error in GetAllThreads: " << error << std::endl;
        // 处理错误
        return;
    }
    for (int i = 0; i < threads_count; i++) {
        // info中的name需要手动释放，其他不用
        jvmtiThreadInfo info;
        // 这个不用手动释放
        jint thread_state_ptr;
        jvmtiError info_error = jvmti->GetThreadInfo(threads[i], &info);
        if (info_error != JVMTI_ERROR_NONE) {
            std::cerr << "Error in GetThreadInfo: " << error << std::endl;
            // 处理错误
            return;
        }
        jvmtiError state_error = jvmti->GetThreadState(threads[i], &thread_state_ptr);
        if (state_error != JVMTI_ERROR_NONE) {
            std::cerr << "Error in GetThreadState: " << error << std::endl;
            // 处理错误
            return;
        }
        threads_trie->insert(info.name);
        thread_names.push_back(info.name);
        thread_states[info.name] = thread_state_ptr;
        jvmti->Deallocate((unsigned char*) info.name);
    }
    // 释放GetAllThreads的指针
    jvmti->Deallocate((unsigned char*) threads);
    //把threads_trie和thread_names传递给groupbythread函数
    groupbyThread(thread_states, threads_trie, thread_names, common_longest_prefix, completed_branches);
    // common_longest_prefix.clear();
    // completed_branches.clear();
    // thread_infos.clear();
    // thread_states.clear();

    delete threads_trie;
}

void trace() {
    // jvmti->RawMonitorEnter(vmtrace_lock);
    std::lock_guard<std::mutex> lock(mtx);
    // fprintf(out, "[%.5f] %s\n", (current_time - start_time) / 1000000000.0, buf);
    statisticThreads();
    
    // jvmti->RawMonitorExit(vmtrace_lock);
}

// void JNICALL VMStart(jvmtiEnv* jvmti, JNIEnv* env) {
//     trace(jvmti);
// }

// void JNICALL VMInit(jvmtiEnv* jvmti, JNIEnv* env, jthread thread) {
//     trace(jvmti);
// }

// void JNICALL VMDeath(jvmtiEnv* jvmti, JNIEnv* env) {
//     trace(jvmti);
// }

void JNICALL ThreadStart(jvmtiEnv* jvmti, JNIEnv* env, jthread thread) {
    trace();
}

void JNICALL ThreadEnd(jvmtiEnv* jvmti, JNIEnv* env, jthread thread) {
    trace();
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM* vm, char* options, void* reserved) {
    vm->GetEnv((void**) &jvmti, JVMTI_VERSION_1_0);

    // jvmti->CreateRawMonitor("vmtrace_lock", &vmtrace_lock);

    // trace();

    // jvmtiCapabilities capabilities = {0};
    // capabilities.can_generate_all_class_hook_events = 1;
    // capabilities.can_generate_compiled_method_load_events = 1;
    // capabilities.can_generate_garbage_collection_events = 1;
    // jvmti->AddCapabilities(&capabilities);

    jvmtiEventCallbacks callbacks = {0};
    // callbacks.VMStart = VMStart;
    // callbacks.VMInit = VMInit;
    // callbacks.VMDeath = VMDeath;
    callbacks.ThreadStart = ThreadStart;
    callbacks.ThreadEnd = ThreadEnd;
    jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));

    // jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, NULL);
    // jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
    // jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, NULL);

    return 0;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM* vm) {
    // if (out != NULL && out != stderr) {
    //     fclose(out);
    // }
}
