#include<iostream>
#include<map>
#include<utility>
#include "offline.h"
#include "deserialize_json_id.h"
using namespace std;
using namespace dmp;

int main(int argc, char* argv[]) {
    JsonTool t1;
    if (argc != 2) {
        cout << "invalid argument" << endl;
        exit(1);
    }
    if (t1.access_files(argv[1])) {
        cout << "Files names successfully retrieved" << endl;
    }
    else {
        cout << "Failure to get file names";
        return 1;
    }
    vector<string> vec = t1.get_file_paths();
    const uint32_t thread_num = vec.size();
    pthread_t threads[thread_num];
    struct thread_parameter param_array[thread_num];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    void* status;
    for (int i = 0; i < thread_num; i++) {
        param_array[i].file_path = vec[i];
        param_array[i].isSucc = false;
        param_array[i].thread_id = i;
        param_array[i].ptr = t1.get_this();
        int rc = pthread_create(&threads[i], NULL, t1.call_thread, (void*)&param_array[i]);
        if (rc) {
            cout << "Failure to create thread: " << i << endl;
            return 1
            return false;
        }
        else {
            cout << "Main() creating thread: " << i << endl;
            //pthread_join(threads[i],NULL);
        }
    }
    for (int i = 0; i < thread_num; i++) {
        int rc = pthread_join(threads[i], &status);
        if (rc) {
            cout << "ERROR" << endl;
            return 1
        }
    }
    //cout << "Main() completed" << endl;
    //pthread_exit(NULL);
	dmpManager m1;
	m1.update_cust_plan_idmap();
	m1.deserialize_mid_txt();
	m1.deserialize_dict_txt();
	m1.update_crowdid_map();
	m1.update_mid_map();
}