#ifndef OFFLINE_HPP
#define OFFLINE_HPP

#include <vector>
#include <map>
#include <stdint.h>
#include <time.h>
#include <set>
#include <utility>
#include <fstream>
#include <sstream>
#include "deserialize_json_id.h"
using namespace std;
namespace dmp {
typedef map<uint32_t, vector<uint32_t> >IdMap;
enum dmpType {
    SET_RATIO = 0,
    SET_RATIO_EXPAND = 1,
    SET_RATIO_ADID = 2,
    SET_RATIO_EXPAND_ADID = 3,
};
struct dict_t {
    float ratio;
    uint32_t crowdid;
    uint32_t custid;
    uint32_t planid;
    bool isExpand;
    vector<uint64_t>adid_vec;
    dict_t():ratio(0.0),crowdid(0),planid(0),isExpand(false){}
};
struct enum_crowdid_t {
    //key: adid; float: ratio
    map<uint64_t, float>adid_ratio_map;
    float ratio;
    bool isExpand;
    enum_crowdid_t():ratio(0.0),isExpand(false){}
};
struct enum_mid_t {
    //key: adid; pair: ratio-crowdid
    map<uint64_t, pair<float, uint32_t> > adid_ratio_map;
    float ratio;
    bool isExpand;
    uint32_t crowdid;
    enum_mid_t():ratio(0.0),isExpand(false), crowdid(0){}
};
//key is enum identifiers
typedef map<dmpType, enum_crowdid_t> enum_crowdid_map;
typedef map<dmpType, enum_mid_t> enum_mid_map;
//key of both map is planid
typedef map<uint32_t, enum_crowdid_map*> plan_crowdid_map;
typedef map<uint32_t, enum_mid_map*> plan_mid_map;
//key is crowdid
typedef map<uint32_t, plan_crowdid_map*> dmp_crowdid_map;
//key is mid
typedef map<uint32_t, plan_mid_map*> dmp_mid_map;


class dmpManager {
    public:
        void update_crowdid_map() {
            cout << "inside update_crowdid_map" << endl;
            for(int i = 0; i < dict_arguments.size(); i++) {
                dict_t* dict = new dict_t(dict_arguments[i]);
                plan_crowdid_map* plan_crowdid_map_ptr = get_plan_crowdid_map_ptr(dict->crowdid);
                if (plan_crowdid_map_ptr != NULL) {
                    enum_crowdid_map* enum_crowdid_map_ptr = get_enum_crowdid_map_ptr(plan_crowdid_map_ptr, dict->planid);
                    if (enum_crowdid_map_ptr != NULL) {
                        update_dmpType_info(dict, enum_crowdid_map_ptr);
                    }
                    else {       
                        enum_crowdid_map* enum_crowdid_map_ptr = new enum_crowdid_map;
                        add_enum_crowdid_map(dict,);
                        plan_crowdid_map_ptr->insert(make_pair(dict->planid, *bottom_map));
                        cout << "Successfully inserted planid: " << i + 1 <<endl;
                        delete bottom_map;
                    }
                }
                else {
                    cout << "Ready to insert dict: " << i + 1 << endl;
                    enum_crowdid_map* bottom_map = new enum_crowdid_map;
                    plan_crowdid_map* middle_map = new plan_crowdid_map;
                    add_plan_crowdid_map(dict, bottom_map, middle_map);
                    crowdid_map.insert(make_pair(dict->crowdid, linux smiddle_map));
                    cout << "Successfully inserted dict: " << i+1 << endl;
                    delete bottom_map;
                    delete middle_map;
                }
                delete dict;
            }
        }
        void update_dmpType_info(dict_t* dict, enum_crowdid_map* ptr) {
            cout << "Inside update_dmpType_info" <<endl;
            bool is_SET_RATIO = false;
            bool is_SET_RATIO_EXPAND = false;
            float set_ratio_val = 0.0;
            float set_ratio_expand_val = 0.0;
            //SET_RATIO
            enum_crowdid_map::iterator it = ptr->find(SET_RATIO);
            if (dict->ratio != 0.0 && !dict->isExpand && it != ptr->end()) {
               is_SET_RATIO = true;
               if (dict->ratio > it->second.ratio) {
                   it->second.ratio = dict->ratio;
                   set_ratio_val = dict->ratio;
                }
            }
            if (dict->ratio != 0.0 && it == ptr->end()) {
                enum_crowdid_t crowdid_t;
                crowdid_t.ratio = dict->ratio;
                ptr->insert(make_pair(SET_RATIO,crowdid_t));
                is_SET_RATIO = true;
                set_ratio_val = dict->ratio;
            }

            //SET_RATIO_EXPAND
            it = ptr->find(SET_RATIO_EXPAND);
            if (dict->ratio != 0.0 && dict->isExpand && it != ptr->end()) {
                is_SET_RATIO_EXPAND = true;
                if (dict->ratio > it->second.ratio) {
                    it->second.ratio = dict->ratio;
                    set_ratio_expand_val = dict->ratio;
                }
            }
            if (dict->ratio != 0.0 && dict->isExpand && it == ptr->end()) {
                enum_crowdid_t crowdid_t;
                crowdid_t.ratio = dict->ratio;
                crowdid_t.isExpand = 1;
                ptr->insert(make_pair(SET_RATIO_EXPAND,crowdid_t));
                is_SET_RATIO_EXPAND = true;
                set_ratio_expand_val = dict->ratio;
            }
            //SET_RATIO_ADID
            it = ptr->find(SET_RATIO_ADID);
            if (!dict->adid_vec.empty() && !dict->isExpand && it != ptr->end()) {
                for (int i = 0; i < dict->adid_vec.size(); i++) {
                    map<uint64_t, float>::iterator adid_ratio_map_it = it->second.adid_ratio_map.find(dict->adid_vec[i]);
                    if (adid_ratio_map_it != it->second.adid_ratio_map.end() && dict->ratio > adid_ratio_map_it->second) {
                        adid_ratio_map_it->second = dict->ratio;
                    }
                    else {
                        if (!is_SET_RATIO) {
                            it->second.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                        }
                        else {
                            if (dict->ratio > set_ratio_val){
                                it->second.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                            }
                        }
                    }
                }
            }
            if (!dict->adid_vec.empty() && !dict->isExpand && it == ptr->end()) {
                enum_crowdid_t crowdid_t;
                 for (int i = 0; i < dict->adid_vec.size(); i++) {
                     if (is_SET_RATIO) {
                        if (dict->ratio > set_ratio_val) {
                            crowdid_t.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                        }
                     }
                     else {
                        crowdid_t.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                     }
                 }
                 ptr->insert(make_pair(SET_RATIO_ADID,crowdid_t));
            }         
            //SET_RATIO_EXPAND_ADID       
            it = ptr ->find(SET_RATIO_EXPAND_ADID);
            if (!dict->adid_vec.empty() && dict->isExpand && it != ptr->end()) {
                for (int i = 0; i < dict->adid_vec.size(); i++) {
                    map<uint64_t, float>::iterator adid_ratio_map_it = it->second.adid_ratio_map.find(dict->adid_vec[i]);
                    if (adid_ratio_map_it != it->second.adid_ratio_map.end() && dict->ratio > adid_ratio_map_it->second) {
                        adid_ratio_map_it->second = dict->ratio;
                    }
                    else {
                        if (!is_SET_RATIO_EXPAND) {
                            it->second.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                        }
                        else {
                            if (dict->ratio > set_ratio_expand_val){
                                it->second.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                            }
                        }
                    }
                }
            }
            if (!dict->adid_vec.empty() && dict->isExpand && it != ptr->end()) {
                enum_crowdid_t crowdid_t;
                crowdid_t.isExpand = 1;
                for (int i = 0; i < dict->adid_vec.size(); i++) {
                     if (is_SET_RATIO_EXPAND) {
                        if (dict->ratio > set_ratio_expand_val) {
                            crowdid_t.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                        }
                     }
                     else {
                        crowdid_t.adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                     }
                 }
                 ptr->insert(make_pair(SET_RATIO_EXPAND_ADID,crowdid_t));
            }
        }
        void add_plan_crowdid_map(dict_t* dict, enum_crowdid_map* bottom_map_, plan_crowdid_map* middle_map_) {
            cout << "Inside add_plan_crowdid_map" << endl;
            add_enum_crowdid_map(dict, bottom_map_);
            middle_map_.insert(make_pair(dict->planid, bottom_map_));
        }
        //can be combined into update_enum_crowdid_map
        void add_enum_crowdid_map(dict_t* dict, enum_crowdid_map* bottom_map_) {
            cout << "Inside add_enum_crowdid_map" << endl;
            enum_crowdid_t* crowdid_t = new enum_crowdid_t;
            bool isRatio = false;
            bool isExpand = false;
            bool isAdid = false;
   
            if (!dict->adid_vec.empty()) {
                for(int i = 0; i < dict->adid_vec.size(); i++) {
                    crowdid_t->adid_ratio_map.insert(make_pair(dict->adid_vec[i],dict->ratio));
                }

                cout << "Checkpoint 1" <<endl;
                isAdid = true;
            }
            else{
                crowdid_t->ratio = dict->ratio;
                isRatio = true;
            }   
            //cout << "CheckPoint 2" << endl;
            if (dict->isExpand) {
                crowdid_t->isExpand = dict->isExpand;
                isExpand = true;
            }
            if (isExpand && isAdid) {
                bottom_map_->insert(make_pair(SET_RATIO_EXPAND_ADID,*crowdid_t));
                cout << "Checkpoint 2" << endl;
            }
            else if (isAdid && !isExpand) {
                bottom_map_->insert(make_pair(SET_RATIO_ADID,*crowdid_t));
            }
            else if (isRatio && isExpand) {
                bottom_map_->insert(make_pair(SET_RATIO_EXPAND,*crowdid_t));
            }
            else if (isRatio && !isExpand) {
                bottom_map_->insert(make_pair(SET_RATIO,*crowdid_t));
            }
            delete crowdid_t;
        }
        //getters;
        plan_crowdid_map* get_plan_crowdid_map_ptr(uint32_t crowdid_) {
            dmp_crowdid_map::iterator it = crowdid_map.find(crowdid_);
            if (it != crowdid_map.end()) {
                return it->second;
            }
            else {
                return NULL;
            }
        }
        enum_crowdid_map* get_enum_crowdid_map_ptr(plan_crowdid_map* plan_crowdid_map_ptr_, uint32_t planid_) const {
            plan_crowdid_map::iterator it = plan_crowdid_map_ptr_->find(planid_);
            if (it != plan_crowdid_map_ptr_->end()) {
                return it->second;
            }
            else {
                return NULL;
            }
        }



        
        //Creating a new map with the key being mid
        bool update_mid_map() { 
            if (!mid_map.empty()){
                mid_map.clear();
            }
            IdMap::iterator it = m_crowd_idmap.begin();
            for (it; it != m_crowd_idmap.end(); it++) { //iteratrate through every mid
                vector<uint32_t>::iterator crowdid_it = it->second.begin();
                //vector<uint32_t> crowdid_vec; //used to store the planid
                plan_mid_map middle_map;//every midid has a middle_map
                for (crowdid_it; crowdid_it != it->second.end(); crowdid_it++) { //iterate through vector that contains crowdid
                    plan_crowdid_map* ptr = get_plan_crowdid_map_ptr(*crowdid_it); //check if the planid is in dmp
                    if (ptr == NULL) {
                        return false;
                    }
                    plan_crowdid_map::iterator plan_crowdid_map_it= ptr->begin(); //iterator for plan_crowdid_map
                    for (plan_crowdid_map_it; plan_crowdid_map_it != ptr->end(); plan_crowdid_map_it++){
                        uint32_t current_planid = plan_crowdid_map_it->first;
                        plan_mid_map::iterator plan_mid_map_it = middle_map.find(plan_crowdid_map_it->first);//iterator for the plan_mid_map
                        if (plan_mid_map_it != middle_map.end()) { //if same planid is found, update the value of plan_mid_map
                            enum_mid_map* ptr_1 = plan_mid_map_it->second;
                            enum_crowdid_map* ptr_2 = get_enum_crowdid_map_ptr(ptr, current_planid);
                            update_enum_map_same_planid(*crowdid_it, ptr_1, ptr_2);
                        }
                        else { //when a new planid is needed to be inserted in plan_mid_map
                            enum_crowdid_map* ptr_2 = get_enum_crowdid_map_ptr(ptr, current_planid);
                            enum_mid_map bottom_map;
                            generate_enum_map_new_planid(*crowdid_it, ptr_2, bottom_map);
                            middle_map.insert(make_pair(plan_crowdid_map_it->first,&bottom_map));
                        }
                    }
                } 
                mid_map.insert(make_pair(it->first, &middle_map));
            }
                        //plan_mid_map::iterator it_ = middle_map.find(crowdid_map_it->second.be)
                //m_crowd_dmpMap.insert(it->first, crowdid_vec);
        }
        bool generate_enum_map_new_planid(uint32_t crowdid, enum_crowdid_map* ptr, enum_mid_map& bottom_map_){
            enum_crowdid_map::iterator it;
            it = ptr->find(SET_RATIO);
            if (it != ptr->end()) {
                enum_mid_t mid_t;
                mid_t.ratio = it->second.ratio;
                mid_t.crowdid = crowdid;
                bottom_map_.insert(make_pair(SET_RATIO,mid_t));
            }
            it = ptr->find(SET_RATIO_EXPAND);
            if (it != ptr->end()) {
                enum_mid_t mid_t;
                mid_t.ratio = it->second.ratio;
                mid_t.crowdid = crowdid;
                mid_t.isExpand = 1;
                bottom_map_.insert(make_pair(SET_RATIO_EXPAND,mid_t));
            }
            it = ptr->find(SET_RATIO_ADID);
            if (it != ptr->end()) {
                enum_mid_t mid_t;
                map<uint64_t, float>::iterator map_it = it->second.adid_ratio_map.begin();
                for(map_it; map_it != it->second.adid_ratio_map.end(); map_it++ ) {
                    mid_t.adid_ratio_map.insert(make_pair(map_it->first,make_pair(map_it->second,crowdid)));
                }
                bottom_map_.insert(make_pair(SET_RATIO_ADID,mid_t));
            }
            it = ptr->find(SET_RATIO_EXPAND_ADID);
            if (it != ptr->end()) {
                enum_mid_t mid_t;
                map<uint64_t, float>::iterator map_it = it->second.adid_ratio_map.begin();
                for(map_it; map_it != it->second.adid_ratio_map.end(); map_it++ ) {
                    mid_t.adid_ratio_map.insert(make_pair(map_it->first,make_pair(map_it->second,crowdid)));
                }
                mid_t.isExpand = 1;
                bottom_map_.insert(make_pair(SET_RATIO_EXPAND_ADID,mid_t));
            }
        }

        bool update_enum_map_same_planid(uint32_t crowdid, enum_mid_map* ptr1, enum_crowdid_map* ptr2) {
            enum_crowdid_map::iterator crowdid_it;
            enum_mid_map::iterator mid_it;
            //SET_RATIO
            crowdid_it = ptr2->find(SET_RATIO);
            mid_it = ptr1->find(SET_RATIO);
            bool is_SET_RATIO = false;
            bool is_SET_RATIO_EXPAND = false;
            float set_ratio_val = 0.0;
            float set_ratio_expand_val = 0.0;
            if (crowdid_it != ptr2->end() && mid_it != ptr1->end()) {
                if (crowdid_it->second.ratio > mid_it->second.ratio) {
                    mid_it->second.crowdid = crowdid;
                    mid_it->second.ratio = crowdid_it->second.ratio;
                    is_SET_RATIO = true;
                    set_ratio_val = mid_it->second.ratio;
                }
            }
            if (crowdid_it != ptr2->end() && mid_it == ptr1->end()) {
                enum_mid_t mid_t;
                mid_t.ratio = crowdid_it->second.ratio;
                mid_t.crowdid = crowdid;
                ptr1->insert(make_pair(SET_RATIO,mid_t));
                is_SET_RATIO = true;
                set_ratio_val = crowdid_it->second.ratio;
            }

            crowdid_it = ptr2->find(SET_RATIO_EXPAND);
            mid_it = ptr1->find(SET_RATIO_EXPAND);
            if (crowdid_it != ptr2->end() && mid_it != ptr1->end()) {
                if (crowdid_it->second.ratio > mid_it->second.ratio) {
                    mid_it->second.crowdid = crowdid;
                    mid_it->second.ratio = crowdid_it->second.ratio;
                    is_SET_RATIO_EXPAND = true;
                    set_ratio_expand_val = mid_it->second.ratio;
                }
            }
            if (crowdid_it != ptr2->end() && mid_it == ptr1->end()) {
                enum_mid_t mid_t;
                mid_t.ratio = crowdid_it->second.ratio;
                mid_t.crowdid = crowdid;
                mid_t.isExpand = 1;
                ptr1->insert(make_pair(SET_RATIO_EXPAND,mid_t));
                is_SET_RATIO_EXPAND = true;
                set_ratio_expand_val = crowdid_it->second.ratio;
            }
            crowdid_it = ptr2->find(SET_RATIO_ADID);
            mid_it = ptr1->find(SET_RATIO_ADID);
            if (crowdid_it != ptr2->end() && mid_it != ptr1->end()) {
                map<uint64_t,float>::iterator it = crowdid_it->second.adid_ratio_map.begin();
                for(it; it != crowdid_it->second.adid_ratio_map.end(); it++) {
                    map<uint64_t, pair<float, uint32_t> >::iterator it_2 = mid_it->second.adid_ratio_map.find(it->first);
                    if (it_2 != mid_it->second.adid_ratio_map.end() && it->second > it_2->second.first) {
                        it_2->second.first = it->second;
                        it_2->second.second = crowdid;
                        //adid remain unchanged
                    }
                    else {
                        if (!is_SET_RATIO) {
                            mid_it->second.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                        }
                        else {
                            if (it->second > set_ratio_val) {
                                mid_it->second.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                            }
                        }
                    }
                }
            }
            if (crowdid_it != ptr2->end() && mid_it == ptr1->end()) {
                map<uint64_t,float>::iterator it = crowdid_it->second.adid_ratio_map.begin();
                enum_mid_t mid_t;
                for(it; it != crowdid_it->second.adid_ratio_map.end(); it++) {
                    if (!is_SET_RATIO) {
                        mid_t.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                    }
                    else {
                        if (it->second > set_ratio_val) {
                             mid_t.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                        }
                    }
                }
                ptr1->insert(make_pair(SET_RATIO_ADID,mid_t));
            }
            crowdid_it = ptr2->find(SET_RATIO_EXPAND_ADID);
            mid_it = ptr1->find(SET_RATIO_EXPAND_ADID);
            if (crowdid_it != ptr2->end() && mid_it != ptr1->end()) {
                map<uint64_t,float>::iterator it = crowdid_it->second.adid_ratio_map.begin();
                for(it; it != crowdid_it->second.adid_ratio_map.end(); it++) {
                    map<uint64_t, pair<float, uint32_t> >::iterator it_2 = mid_it->second.adid_ratio_map.find(it->first);
                    if (it_2 != mid_it->second.adid_ratio_map.end() && it->second > it_2->second.first) {
                        it_2->second.first = it->second;
                        it_2->second.second = crowdid;
                    }
                    else {
                        if (!is_SET_RATIO_EXPAND) {
                            mid_it->second.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                        }
                        else {
                            if (it->second > set_ratio_expand_val) {
                                mid_it->second.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                            }
                        }
                    }
                }
            }
            if (crowdid_it != ptr2->end() && mid_it == ptr1->end()) {
                map<uint64_t,float>::iterator it = crowdid_it->second.adid_ratio_map.begin();
                enum_mid_t mid_t;
                mid_t.isExpand = 1;
                for(it; it != crowdid_it->second.adid_ratio_map.end(); it++) {
                    if (!is_SET_RATIO_EXPAND) {
                        mid_t.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                    }
                    else {
                        if (it->second > set_ratio_expand_val) {
                            mid_t.adid_ratio_map.insert(make_pair(it->first,make_pair(it->second,crowdid)));
                        }
                    }
                }
                ptr1->insert(make_pair(SET_RATIO_ADID,mid_t));
            }
        }    
        bool deserialize_dict_txt() {
            ifstream ins;
            string file_name = "search_crowdid_planid_adid.txt";
            ins.open(file_name.c_str());
            if (ins.fail()) {
                cout << "Input file opening failed" << endl;
                return false;
            }
            else {
                cout << "File opened successfully" << endl;
            }
            string line;
            vector<vector<string> >dicts;
            while (getline(ins, line)) {
                stringstream ss(line);
                string word;
                vector<string>one_line;
                while (ss >> word) {
                    if (word.length() > 1) {
                        uint32_t start_pos = 0;
                        for (uint32_t i = 0; i < word.length(); i++) {
                            if (word[i] == '\x1') {
                                string subword(word, start_pos, i - start_pos);
                                one_line.push_back(subword);
                                start_pos = i + 1;
                            }
                            if (i == word.length() - 1) {
                                string subword(word, start_pos, word.length() - start_pos);
                                one_line.push_back(subword);
                            }
                        }
                    }
                }
                dicts.push_back(one_line);
            }
            ins.close();
            //vector<dict_t>dict_arguments;
            for (int i = 0; i < dicts.size(); i++) {
                dict_t one_dict;
                vector<string>::iterator it = dicts[i].begin();
                //istringstream iss(*it);
                //one_dict.crowdid = static_cast<uint32_t>(*it);
                //one_dict.custid = static_cast<uint32_t>(stoul(*(it + 1)));
                //one_dict.planid = static_cast<uint32_t>(stoul(*(it + 2)));
                //or use boost::lexical_cast;
                istringstream(*it) >> one_dict.crowdid;
                istringstream(*(it+1)) >> one_dict.custid;
                istringstream(*(it+2)) >> one_dict.planid;
                istringstream(*(it + 3)) >> one_dict.isExpand;
                istringstream(*(it+4)) >> one_dict.ratio;
                //one_dict.ratio = stof(*(it + 4));

                /*cout << one_dict.crowdid << endl;
                cout << one_dict.custid << endl;
                cout << one_dict.planid << endl;
                cout << one_dict.isExpand << endl;
                cout << one_dict.ratio << endl;
                */
                it = it + 5;
                for (it; it != dicts[i].end(); it++) {
                    uint64_t adid;
                    istringstream(*it) >> adid;
                    one_dict.adid_vec.push_back(adid);
                }
                dict_arguments.push_back(one_dict);
            }
            return true;
        }
        bool deserialize_mid_txt() {
            map<uint32_t,vector<uint32_t> >mid_crowdid_map;
            ifstream ins;
            ins.open("mid_crowdid.txt");
            if (ins.fail()) {
                cout << "Input file opening failed" << endl;
                return false;
            }
            else {
                cout << "File opened successfully" << endl;
            }
            string line;
            while (getline(ins, line)) {
                uint32_t next_pos = 0;
                vector<uint32_t>crowdid_vec;
                uint32_t m_id;
                size_t found = line.find("001");
                if (found != string::npos) {
                    string mid(line, 0, found - 1);
                    istringstream(mid)>>m_id;
                }
                else {
                    cout << "mid is not found";
                }
                //found is an index number currently
                while (found != string::npos) {
                    uint32_t start_pos = 0;
                    start_pos = 2 + found;
                    found = line.find("002", found + 1);
                    string crowdid(line, start_pos, found - start_pos - 1);
                    //cout << crowdid << endl;
                    uint32_t crowd_id;
                    istringstream(crowdid) >> crowd_id;
                    crowdid_vec.push_back(crowd_id);
                }
                m_crowd_idmap.insert(make_pair(m_id,crowdid_vec));
            }
            ins.close();
        }

        void update_cust_plan_idmap(JsonTool tool_object) {
            cust_plan_idmap = tool_object.get_acc_plan_idmap();
            plan_group_idmap = tool_object.get_plan_grp_idmap();
        }
        /*void display() {
            dmp_crowdid_map::iterator it = crowdid_map.begin();
            for (it; it != crowdid_map.end(); it++) {
                cout << "Crowdid: "<< it->first <<endl;
                plan_crowdid_map::iterator it2 = it->second->begin();
                for (it2; it2 != it->second->end(); it2++) {
                    cout << "Planid: " << it2->first << " ";
                    enum_crowdid_map::iterator it3 = it2->second->begin();
                    for (it3; it3 != it2->second->end(); it3++) {
                        cout << "Case: " << it3->first << " ";
                    }
                    cout << endl;

                }
                cout << endl;
            }

        }
        */
        void display_m_crowdid_map() {
            map<uint32_t,vector<uint32_t> >::iterator it = m_crowd_idmap.begin();
            for (it; it != m_crowd_idmap.end(); it++) {
                cout << "Mid is: " << it->first << " "<<"Crowdid are: ";
                for (int i = 0; i < it->second.size(); i++) {
                    cout << it->second[i] << " ";
                }
                cout << endl;
            }
        }
        void display_dict_arguments() {
            for (int i = 0; i < dict_arguments.size(); i++) {
                cout << "Crowdid: " << dict_arguments[i].crowdid << endl;
                cout << "Custid: " << dict_arguments[i].custid << endl;
                cout << "Planid: " << dict_arguments[i].planid << endl;
                cout << "Expand: " << dict_arguments[i].isExpand << endl;
                cout << "Ratio: " << dict_arguments[i].ratio << endl;
                vector<uint64_t>::iterator it = dict_arguments[i].adid_vec.begin();
                cout << "Adid: ";
                for (it; it != dict_arguments[i].adid_vec.end(); it++) {
                    cout << *it << " ";
                }
                cout << endl;
            }
        }
        
    private:
        dmp_crowdid_map crowdid_map;
        dmp_mid_map mid_map;
        IdMap m_crowd_idmap; //key: mid - value:vector<crowdid>
        IdMap cust_plan_idmap; //key: custid - value:vector<planid>
        IdMap plan_group_idmap; //key: planid - value:vector<groupid>
        vector<dict_t>dict_arguments;
};

}
#endif
