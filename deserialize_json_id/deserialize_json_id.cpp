#include <fstream>
#include <json/json.h>
#include <json/reader.h>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include "deserialize_json_id.h"
using namespace std;
using namespace dmp;
bool JsonTool::access_files(const char* root_directory) {
    sleep(1);
    DIR* root_dir = opendir(root_directory);
    if (root_dir == NULL) {
        cout << "Directory parameter not found" << endl;
        return false;
    }
    struct dirent* root_dirp;
    while ((root_dirp = readdir(root_dir)) != NULL) {
        string name(root_dirp->d_name);
        if (name == "." || name == "..") {
            continue;
        }
        string start_name(root_directory);
        if (start_name.find("/") == string::npos) {
            start_name += "/"; //input/
        }
        start_name += name; //input/dir_1
        DIR* sub_dir = opendir(start_name.c_str());
        if (sub_dir != NULL) {
            struct dirent* dirp;
            while ((dirp = readdir(sub_dir)) != NULL) {
                string file_name(dirp->d_name);
                if (file_name == "." || file_name == "..") {
                    continue;
                }
                size_t found = file_name.find("ad.json");
                if (found != string::npos) {
                    string account_file_path = start_name + "/" + file_name;
                    file_paths.push_back(account_file_path);
                }
            }
        }
        cout << endl;
        closedir(sub_dir);
    }
    closedir(root_dir);
    return true;
}

bool JsonTool::deserialize_id(string file_path) {
    sleep(1);
    ifstream ins;
    cout << file_path << endl;
    ins.open(file_path.c_str());
    if (ins.fail()) {
        cout << "Input file opening failed" << endl;
        return false;
    }
    else {
        cout << "File " << file_path << " opened successfully" << endl;
    }

    int lnum = 0;
    Json::Value value;
    Json::Reader reader;
    string for_j;
    //IdMap account_plan_id_map;
    //IdMap plan_group_id_map;
    while (getline(ins, for_j)) {
        /*if (lnum % 100000 == 0) {
            cout << "have read " << lnum << " lines" << endl;
        }
        */
        bool is_parsed = reader.parse(for_j.c_str(), value);
        if (!is_parsed) {
            cout << "Failed to parse" << reader.getFormattedErrorMessages() << endl;
            return false;
        }
        lnum++;
        uint32_t group_id = value.get("groupId", 0).asUInt();
        uint32_t plan_id = value.get("planId", 0).asUInt();
        uint32_t account_id = value.get("userId", 0).asUInt();
        IdMap::iterator acc_plan_it = acc_plan_idmap.find(account_id);
        IdMap::iterator plan_grp_it = plan_grp_idmap.find(plan_id);
        if (acc_plan_it != account_plan_id_map.end()) {
            acc_plan_it->second.push_back(plan_id);
        }
        else {
            vector<uint32_t> vec;
            vec.push_back(plan_id);
            acc_plan_idmap[account_id] = vec;
        }
        if (plan_grp_it != plan_group_id_map.end()) {
            plan_grp_it->second.push_back(group_id);
        }
        else {
            vector<uint32_t> vec;
            vec.push_back(group_id);
            plan_grp_idmap[plan_id] = vec;
        }
    }
    ins.close();
    //id_info id_object(account_plan_id_map, plan_group_id_map);
    //id_list.push_back(id_object);
    cout << "YES: cust_plan_idmap and plan_group_idmap SCUCCESSFULLY generated" <<endl
    //cout << "File " << file_path << " properly executed" << endl;
    return true;
}

