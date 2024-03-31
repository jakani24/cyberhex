#include "deepscan.h"
#include "virus_ctrl.h"
#include "md5hash.h"
#include "well_known.h"
#include "log.h"
#include "utils.h"
#include "scan.h"
#include "app_ctrl.h"
#include <thread>
#include <iostream>
#include <yara.h>
#include <string>
#include <vector>
#include <stack>
#include <filesystem>
#include <mutex>

std::vector<YR_RULES*> compiled_rules; //global variable to store the compiled rules
std::mutex yara_scan_mutex;
int deep_cnt = 0;
int deep_all_files = 0;
int action_deepscan_is_virus = 0; //flag that is set by the callback function to tell the action_deepscan function if the file is a virus or not. this is needed to talk to the desktop client
//scan with yara rules
//functions to create: action_deepscanfile
//deepscan_file_t
//action_deepscanfolder
//deepscan_folder


YR_RULES* load_yara_rules(const char* ruleFilePath, YR_RULES* compiledRules = nullptr) {
    // Create a new compiler
    YR_COMPILER* compiler;
    if (yr_compiler_create(&compiler) != ERROR_SUCCESS) {
        std::cerr << "Failed to create YARA compiler." << std::endl;
        return nullptr;
    }

    FILE* file;
    fopen_s(&file, ruleFilePath, "r");
    if (file == nullptr) {
        yr_compiler_destroy(compiler);
        return nullptr;
    }
    int result = yr_compiler_add_file(compiler, file, nullptr, ruleFilePath);
    if (result != ERROR_SUCCESS) {
        //std::cerr << "Failed to compile YARA rules from file: " << ruleFilePath << std::endl;
        log(LOGLEVEL::ERR_NOSEND, "[load_yara_rules()]: Failed to compile YARA rules from file: ", ruleFilePath);
        yr_compiler_destroy(compiler);
        return nullptr;
    }

    // Get rules from compiler and add them to the compiledRules object
    yr_compiler_get_rules(compiler, &compiledRules);

    // Destroy the compiler
    yr_compiler_destroy(compiler);

    return compiledRules;
}
void init_yara_rules(const char* folderPath) {

    // Stack to store directories to be traversed iteratively
    std::stack<std::string> directories;
    directories.push(folderPath);

    while (!directories.empty()) {
        std::string currentDir = directories.top();
        directories.pop();

        for (const auto& entry : std::filesystem::directory_iterator(currentDir)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                if (filePath.ends_with(".yar") || filePath.ends_with(".yara")) {
                    YR_RULES* rules = load_yara_rules(filePath.c_str());
                    if (rules != nullptr) {
                        compiled_rules.push_back(rules);
                    }
                }
            }
            else if (entry.is_directory()) {
                directories.push(entry.path().string());
            }
        }
    }
}


std::stack<std::string> deep_directories; // Stack to store directories to be scanned

void deepscan_folder(const std::string& directory) {
    deep_directories.push(directory);

    while (!deep_directories.empty()) {
        std::string current_dir = deep_directories.top();
        deep_directories.pop();

        std::string search_path = current_dir + "\\*.*";
        WIN32_FIND_DATA find_file_data;
        HANDLE hFind = FindFirstFile(search_path.c_str(), &find_file_data);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(find_file_data.cFileName, ".") == 0 || strcmp(find_file_data.cFileName, "..") == 0) {
                    continue; // Skip the current and parent directories
                }

                const std::string full_path = current_dir + "\\" + find_file_data.cFileName;
                if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // If it's a directory, add it to the stack
                    deep_directories.push(full_path);
                }
                else {
                    if (debug_mode())
                        log(LOGLEVEL::INFO_NOSEND, "[deepscan_folder()]: Scanning file: ", full_path);

                    // Do multithreading here
                    int thread_timeout = 0;
                    //log(LOGLEVEL::INFO_NOSEND, "[scan_folder()]: Scanning file: ", full_path);
                    while (get_num_threads() >= std::thread::hardware_concurrency() && thread_safety()) {
                        Sleep(10);
                        thread_timeout++;
                        //printf("Thread timeout: %d\n", thread_timeout);
                        if (thread_timeout == 100 * 20) {
                            // If there is no available thread for more than 30 seconds, reset the thread counter
                            set_num_threads(0);
                        }
                    }
                    //log(LOGLEVEL::INFO_NOSEND, "[scan_folder()]: Scanning file: ", full_path);
                    if (is_valid_path(full_path)) { // Filter out invalid paths and paths with weird characters
                        std::uintmax_t fileSize = std::filesystem::file_size(full_path);
                        if (fileSize > 4000000000) { // 4GB
                            log(LOGLEVEL::INFO_NOSEND, "[deepscan_folder()]: File too large to scan: ", full_path);
                        }
                        else {
                            std::thread scan_thread(deepscan_file_t, full_path);
                            scan_thread.detach();
                        }
                    }
                    else {
                        log(LOGLEVEL::INFO_NOSEND, "[deepscan_folder()]: Invalid path: ", full_path);
                    }
                    deep_cnt++;
                    if (deep_cnt % 100 == 0) {
                        printf("Processed %d files;\n", deep_cnt);
                        //printf("Number of threads: %d\n", num_threads);
                    }
                    if (deep_cnt % 1000 == 0) {
                        int actual_threads = get_num_running_threads();
                        if (get_num_threads() > actual_threads)
                            set_num_threads(actual_threads);//correct value of threads
                        printf("Number of threads: %d\n", get_num_threads());
                        //send progress to com file
                        std::ofstream answer_com(ANSWER_COM_PATH, std::ios::app);
                        if (answer_com.is_open()) {
                            answer_com << "progress " << (deep_cnt * 100 / (deep_all_files + 1)) << "\n";
                            answer_com.close();
                        }
                    }
                }
            } while (FindNextFile(hFind, &find_file_data) != 0);
            FindClose(hFind);
        }
        else {
            log(LOGLEVEL::ERR_NOSEND, "[deepscan_folder()]: Could not open directory: ", current_dir, " while scanning files inside directory.");
        }
    }
}


struct Callback_data {
    std::string filepath;
    // You can add more data members here if needed
};
int process_callback(YR_SCAN_CONTEXT* context,int message, void* message_data, void* user_data) {
    switch (message) {
        case CALLBACK_MSG_RULE_MATCHING:
        {
            // Access filepath from CallbackData
            Callback_data* callback_data = (Callback_data*)user_data;

            // Access filepath from CallbackData
            std::string filepath = callback_data->filepath;
            //we calculate the hash of the file so the virus ctrl functions are able to process it
            std::string hash = md5_file_t(filepath);

            virus_ctrl_store(filepath, hash, hash);
            //afterwards do the processing with that file
            virus_ctrl_process(hash);
            action_deepscan_is_virus = 1;
            break;
        }
    }
	return CALLBACK_CONTINUE;
}
bool deepscan_file_t(const std::string&file_path) {
    set_num_threads(get_num_threads() + 1);
    //we do not need to make a new instance of yara rules, because they are global and do not get deteled or modified
    //std::lock_guard<std::mutex> lock(yara_scan_mutex);
    thread_local std::string file_path_(file_path);
    //get globally set yara rules and iterate over them
    Callback_data* callback_data = new Callback_data();
    for (YR_RULES* rule : compiled_rules) {
        callback_data->filepath = file_path_;
        yr_rules_scan_file(rule, file_path.c_str(), 0, process_callback, callback_data, 5000);
    }
    set_num_threads(get_num_threads() - 1);
    return true;
}

void action_deepscanfolder(const std::string& folderpath) {
    thread_init();
    thread_local std::string folderpath_(folderpath);
    deep_cnt = 0;
    deep_all_files = get_num_files(folderpath_);
    //tell the desktop client that the scan has started
    std::ofstream answer_com1(ANSWER_COM_PATH, std::ios::app);
    if (answer_com1.is_open()) {
        answer_com1 << "start " << deep_all_files << "\n";
        answer_com1.close();
    }
    deepscan_folder(folderpath_);
    std::ofstream answer_com(ANSWER_COM_PATH, std::ios::app);
    if (answer_com.is_open()) {
        answer_com << "end " << "\"" << "nothing" << "\"" << " " << "nothing" << " " << "nothing" << "\n";
        answer_com.close();
    }
    thread_shutdown();
}

//for singlethreaded scans
void action_deepscanfile(const std::string& filepath_) {
    thread_init();
    std::string filepath(filepath_);
    char* db_path = new char[300];
    char* hash = new char[300];
    action_deepscan_is_virus = 0;
    //printf("start\n");
    if (is_valid_path(filepath)) { //filter out invalid paths and paths with weird characters
        deepscan_file_t(filepath);
        if (action_deepscan_is_virus == 0) {
            std::ofstream answer_com(ANSWER_COM_PATH, std::ios::app);
            if (answer_com.is_open()) {
                answer_com << "not_found " << "\"" << filepath << "\"" << " " << hash << " " << "no_action_taken" << "\n";
                answer_com.close();
            }
        }
    }
    else
        log(LOGLEVEL::INFO_NOSEND, "[action_scanfile()]: Invalid path: ", filepath_);
    thread_shutdown();
}