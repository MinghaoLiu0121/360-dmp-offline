#ifndef DESERIALIZE_JSON_FILE_HPP
#define DESERIALIZE_JSON_FILE_HPP
#include <stdint.h>
#include <map>
#include <string>
#include <list>
#include <vector>
#include <pthread.h>

using namespace std;
namespace dmp{
    //unsigned int 0 - 4294967295
typedef map<uint32_t, vector<uint32_t> >IdMap;

class JsonTool;
struct id_info {
    IdMap acc_plan_id;
    IdMap plan_grp_id;
    id_info(IdMap map1, IdMap map2) :acc_plan_id(map1), plan_grp_id(map2) {}
};
struct thread_parameter {
    string file_path;
    bool isSucc;
    uint32_t thread_id;
    JsonTool* ptr;
};

class JsonTool {
public:
    JsonTool();
    bool access_files(const char* root_directory);
    bool deserialize_id(std::string file_path);
    static void* call_thread(void* param) {
        sleep(1);
        struct thread_parameter* param_ptr = static_cast<struct thread_parameter*>(param);
        JsonTool* ptr = param_ptr->ptr;
        param_ptr->isSucc = ptr->deserialize_id(param_ptr->file_path);
    }
    JsonTool* get_this() {
        return this;
    }
    vector<string>  get_file_paths() const {
        return file_paths;
    }
    IdMap get_acc_plan_idmap() {
        return acc_plan_idmap;
    }
    IdMap get_plan_grp_idmap() {
        return plan_grp_idmap;
    }
    //a potential getter for id_list
private:
    //list<id_info>id_list;
    IdMap acc_plan_idmap;
    IdMap plan_grp_idmap;
    vector<string>file_paths;

};
}
#endif
