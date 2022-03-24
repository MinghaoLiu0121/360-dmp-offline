#include <iostream>
#include <utility>
#include <vector>
#include <map>
#include <stdint.h>
#include <string>
#include <time.h>
#include <set>
#include <utility>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <json/json.h>
#include <iostream>
#include <sys/types.h>
#include <gflags/gflags.h>
#include "com/public/util_v2/StringUtils.cpp"
#include "boost/lexical_cast.hpp"

using namespace std;
DEFINE_string(inputpath, "./input", "default input data path");
DEFINE_string(outputpath, "./output", "default output data path");
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
    uint32_t isExpand;
    uint64_t adid;
    dict_t():ratio(0.0),crowdid(0),planid(0),isExpand(0){}
};
struct dict_plan_detail_t {
    //key: adid; float: ratio
    map<uint64_t, float>adid_ratio_map;
    float ratio;
    uint32_t isExpand;
    dict_plan_detail_t():ratio(0.0),isExpand(0){}
};
struct mid_plan_detail_t {
    //key: adid; pair: ratio-crowdid
    map<uint64_t, pair<float, uint32_t> > adid_ratio_map;
    float ratio;
    uint32_t isExpand;
    uint32_t crowdid;
    mid_plan_detail_t():ratio(0.0),isExpand(0), crowdid(0){}
};
typedef map<uint32_t, vector<uint32_t> >IdMap;
//for dict trans
typedef dict_plan_detail_t DICT_PLAN_DETIAL_T;
typedef map<dmpType, DICT_PLAN_DETIAL_T> DICT_PLAN_DETAIL_MAP;
typedef map<uint32_t, DICT_PLAN_DETAIL_MAP> DICT_PLAN_MAP;
//for mid trans
typedef mid_plan_detail_t MID_PLAN_DETAIL_T;
typedef map<dmpType, MID_PLAN_DETAIL_T> MID_PLAN_DETAIL_MAP;
typedef map<uint32_t, MID_PLAN_DETAIL_MAP> MID_PLAN_MAP;
typedef map<string, MID_PLAN_MAP> MID_MAP;


void update_crowdid_map(vector<dict_t>& dict_vec, map<uint32_t, vector<uint32_t> >& cust_plan_map, map<uint32_t, DICT_PLAN_MAP>& crowdid_planid_map) {
    for (int i = 0; i < dict_vec.size(); i++) {
        dict_t& dict_i = dict_vec[i];
        // prepare planid
        vector<uint32_t> planid;
        if (dict_i.planid == 0 && cust_plan_map.find(dict_i.custid) == cust_plan_map.end()) {
            continue;
        } else if (dict_i.planid == 0 && cust_plan_map.find(dict_i.custid) != cust_plan_map.end()) {
            planid = cust_plan_map[dict_i.custid];
        } else if (dict_i.planid != 0) {
            planid.push_back(dict_i.planid); 
        }

        for (int i = 0; i < planid.size(); ++i) {
            dict_i.planid = planid[i];
            //crowdid
            if (crowdid_planid_map.find(dict_i.crowdid) == crowdid_planid_map.end()) {
                DICT_PLAN_MAP dict_plan_map;
                crowdid_planid_map.insert(make_pair(dict_i.crowdid, dict_plan_map));
            }
            DICT_PLAN_MAP& dict_plan_map = crowdid_planid_map[dict_i.crowdid];
            //planid
            if (dict_plan_map.find(dict_i.planid) == dict_plan_map.end()) {
                DICT_PLAN_DETAIL_MAP dict_plan_detail_map;
                dict_plan_map.insert(make_pair(dict_i.planid, dict_plan_detail_map));
            }
            DICT_PLAN_DETAIL_MAP& dict_plan_detail_map = dict_plan_map[dict_i.planid];
            //type
            if (dict_i.isExpand == 0 && dict_i.adid == 0) {
                if (dict_plan_detail_map[SET_RATIO].ratio < dict_i.ratio) {
                    dict_plan_detail_map[SET_RATIO].ratio = dict_i.ratio;
                }
            } else if (dict_i.isExpand == 1 && dict_i.adid == 0) {
                if (dict_plan_detail_map[SET_RATIO_EXPAND].ratio < dict_i.ratio) {
                    dict_plan_detail_map[SET_RATIO_EXPAND].ratio = dict_i.ratio;
                    dict_plan_detail_map[SET_RATIO_EXPAND].isExpand = dict_i.isExpand;
                }
            } else if (dict_i.isExpand == 0 && dict_i.adid != 0) {
                if (dict_plan_detail_map[SET_RATIO_ADID].adid_ratio_map[dict_i.adid] < dict_i.ratio) {
                    dict_plan_detail_map[SET_RATIO_ADID].adid_ratio_map[dict_i.adid] = dict_i.ratio;
                }
            } else if (dict_i.isExpand == 1 && dict_i.adid != 0) {
                if (dict_plan_detail_map[SET_RATIO_EXPAND_ADID].adid_ratio_map[dict_i.adid] < dict_i.ratio) {
                    dict_plan_detail_map[SET_RATIO_EXPAND_ADID].adid_ratio_map[dict_i.adid] = dict_i.ratio;
                    dict_plan_detail_map[SET_RATIO_EXPAND_ADID].isExpand = dict_i.isExpand;
                }
            }
        }
    }
}
//Creating a new map with the key being mid
bool update_mid_map(map<uint32_t, DICT_PLAN_MAP>& crowdid_planid_map, map<string,vector<uint32_t> >* mid_crowdid_map, MID_MAP& mid_map) {
    for (map<string, vector<uint32_t> >::iterator it = mid_crowdid_map->begin(); it != mid_crowdid_map->end(); ++it) {
        vector<uint32_t>& crowdid_vec = it->second;
        for (int i = 0; i < crowdid_vec.size(); ++i) {
            uint32_t crowdid = crowdid_vec[i];
            if (crowdid_planid_map.find(crowdid) == crowdid_planid_map.end()) continue;
            else {
                // dict data
                DICT_PLAN_MAP& dict_plan_map = crowdid_planid_map[crowdid];
                // mid
                const string& mid = it->first;
                if (mid_map.find(mid) == mid_map.end()) {
                    MID_PLAN_MAP mid_plan_map;
                    mid_map.insert(make_pair(mid, mid_plan_map));
                }
                MID_PLAN_MAP& mid_plan_map = mid_map[mid];
                for (DICT_PLAN_MAP::iterator dict_plan_map_it = dict_plan_map.begin(); dict_plan_map_it != dict_plan_map.end(); ++dict_plan_map_it) {
                    uint32_t planid = dict_plan_map_it->first;
                    DICT_PLAN_DETAIL_MAP& dict_plan_detail_map = dict_plan_map_it->second;
                    // plan
                    if (mid_plan_map.find(planid) == mid_plan_map.end()) {
                        MID_PLAN_DETAIL_MAP mid_plan_detail_map;
                        mid_plan_map.insert(make_pair(planid, mid_plan_detail_map));
                    }
                    MID_PLAN_DETAIL_MAP& mid_plan_detail_map = mid_plan_map[planid];
                    for (DICT_PLAN_DETAIL_MAP::iterator dict_plan_detail_map_it = dict_plan_detail_map.begin(); dict_plan_detail_map_it != dict_plan_detail_map.end(); ++dict_plan_detail_map_it) {
                        dmpType type = dict_plan_detail_map_it->first;
                        DICT_PLAN_DETIAL_T& dict_plan_detail = dict_plan_detail_map_it->second;
                        if (type == SET_RATIO) {
                            if (mid_plan_detail_map[type].ratio < dict_plan_detail.ratio) {
                                mid_plan_detail_map[type].ratio = dict_plan_detail.ratio;
                                mid_plan_detail_map[type].crowdid = crowdid;
                            }
                        } else if (type == SET_RATIO_EXPAND) {
                            if (mid_plan_detail_map[type].ratio < dict_plan_detail.ratio) {
                                mid_plan_detail_map[type].ratio = dict_plan_detail.ratio;
                                mid_plan_detail_map[type].crowdid = crowdid;
                                mid_plan_detail_map[type].isExpand = dict_plan_detail.isExpand;
                            }
                        } else if (type == SET_RATIO_ADID) {
                            for (map<uint64_t, float>::iterator adid_it = dict_plan_detail.adid_ratio_map.begin(); adid_it != dict_plan_detail.adid_ratio_map.end(); ++adid_it) {
                                uint64_t adid = adid_it->first;
                                float ratio = adid_it->second;
                                // adid
                                if (mid_plan_detail_map[type].adid_ratio_map.find(adid) == mid_plan_detail_map[type].adid_ratio_map.end()) {
                                    pair<float, uint32_t> ratio_crowdid;
                                    mid_plan_detail_map[type].adid_ratio_map.insert(make_pair(adid, ratio_crowdid));
                                }
                                pair<float, uint32_t>& ratio_crowdid = mid_plan_detail_map[type].adid_ratio_map[adid];
                                if (ratio_crowdid.first < ratio) {
                                    ratio_crowdid.first = ratio;
                                    ratio_crowdid.second = crowdid;
                                }
                            }
                        } else {
                            for (map<uint64_t, float>::iterator adid_it = dict_plan_detail.adid_ratio_map.begin(); adid_it != dict_plan_detail.adid_ratio_map.end(); ++adid_it) {
                                uint64_t adid = adid_it->first;
                                float ratio = adid_it->second;
                                // adid
                                if (mid_plan_detail_map[type].adid_ratio_map.find(adid) == mid_plan_detail_map[type].adid_ratio_map.end()) {
                                    pair<float, uint32_t> ratio_crowdid;
                                    mid_plan_detail_map[type].adid_ratio_map.insert(make_pair(adid, ratio_crowdid));
                                }
                                pair<float, uint32_t>& ratio_crowdid = mid_plan_detail_map[type].adid_ratio_map[adid];
                                if (ratio_crowdid.first < ratio) {
                                    ratio_crowdid.first = ratio;
                                    ratio_crowdid.second = crowdid;
                                }
                            }
                            mid_plan_detail_map[type].isExpand = dict_plan_detail.isExpand;
                        }
                    }
                }
            }
        }
    }
}
void delete_useless_adid(MID_MAP& mid_map) {
    for (MID_MAP::iterator it = mid_map.begin(); it != mid_map.end(); ++it) {
        MID_PLAN_MAP& mid_plan_map = it->second;
        for (MID_PLAN_MAP::iterator mid_plan_map_it = mid_plan_map.begin(); mid_plan_map_it != mid_plan_map.end(); ++mid_plan_map_it) {
            MID_PLAN_DETAIL_MAP& mid_plan_detail_map = mid_plan_map_it->second;
            // case 0, case 2
            if (mid_plan_detail_map.find(SET_RATIO_ADID) != mid_plan_detail_map.end() && mid_plan_detail_map.find(SET_RATIO) != mid_plan_detail_map.end()) {
                for (map<uint64_t, pair<float, uint32_t> >::iterator adid_it = mid_plan_detail_map[SET_RATIO_ADID].adid_ratio_map.begin(); adid_it != mid_plan_detail_map[SET_RATIO_ADID].adid_ratio_map.end(); ) {
                    if (adid_it->second.first <= mid_plan_detail_map[SET_RATIO].ratio) {
                        mid_plan_detail_map[SET_RATIO_ADID].adid_ratio_map.erase(adid_it++);
                    } else {
                        adid_it++;
                    }
                }
                if (mid_plan_detail_map[SET_RATIO_ADID].adid_ratio_map.size() == 0) {
                    mid_plan_detail_map.erase(SET_RATIO_ADID);
                }
            }
            // case 1, case 3
            if (mid_plan_detail_map.find(SET_RATIO_EXPAND_ADID) != mid_plan_detail_map.end() && mid_plan_detail_map.find(SET_RATIO_EXPAND) != mid_plan_detail_map.end()) {
                for (map<uint64_t, pair<float, uint32_t> >::iterator adid_it = mid_plan_detail_map[SET_RATIO_EXPAND_ADID].adid_ratio_map.begin(); adid_it != mid_plan_detail_map[SET_RATIO_EXPAND_ADID].adid_ratio_map.end(); ) {
                    if (adid_it->second.first <= mid_plan_detail_map[SET_RATIO_EXPAND].ratio) {
                        mid_plan_detail_map[SET_RATIO_EXPAND_ADID].adid_ratio_map.erase(adid_it++);
                    } else {
                        adid_it++;
                    }
                }
                if (mid_plan_detail_map[SET_RATIO_EXPAND_ADID].adid_ratio_map.size() == 0) {
                    mid_plan_detail_map.erase(SET_RATIO_EXPAND_ADID);
                }
            }
        }
    }
}
void trans_ad_2_str(map<uint64_t, pair<float, uint32_t> >& adid_ratio_map, string& adid) {
    map<pair<float, uint32_t>, vector<uint64_t> > m;
    for (map<uint64_t, pair<float, uint32_t> >::iterator ratio_it = adid_ratio_map.begin(); ratio_it != adid_ratio_map.end(); ++ratio_it) {
        if (m.find(ratio_it->second) == m.end()) {
            vector<uint64_t> v;
            m.insert(make_pair(ratio_it->second, v));
        }
        vector<uint64_t>& v = m[ratio_it->second];
        v.push_back(ratio_it->first);
    }
    bool ratio_first = true;
    for (map<pair<float, uint32_t>, vector<uint64_t> >::iterator adid_it = m.begin(); adid_it != m.end(); ++adid_it) {
        if (ratio_first) {
            ratio_first = false;
        } else {
            adid.append("\006");
        }
        adid.append(boost::lexical_cast<string>(adid_it->first.first)).append("\004");
        adid.append(boost::lexical_cast<string>(adid_it->first.second)).append("\004");
        bool adid_first = true;
        for (int k = 0; k < adid_it->second.size(); ++k) {
            if (adid_first) {
                adid_first = false;
            } else {
                adid.append("\005");
            }
            adid.append(boost::lexical_cast<string>(adid_it->second[k]));
        }
    }
}
void serialize_mid(MID_MAP& mid_map) {
    string line;
    string plan;
    string adid;
    string file_name = FLAGS_outputpath + "mid_result.txt";
    ofstream ofs(file_name.c_str());
    if (!ofs.is_open()) {
        cout << "mid_result file opening failed" << endl;
        exit(1);
    } else {
        cout << "mid_result opened successfully" << endl;
    }
    for (MID_MAP::iterator it = mid_map.begin(); it != mid_map.end(); ++it) {
        line.clear();
        const string& mid = it->first;
        line.append(mid).append("\001");
        MID_PLAN_MAP& mid_plan_map = it->second;
        bool plan_first = true;
        for (MID_PLAN_MAP::iterator mid_plan_map_it = mid_plan_map.begin(); mid_plan_map_it != mid_plan_map.end(); ++mid_plan_map_it) {
            plan.clear();
            if (plan_first) {
                plan_first = false;
            } else {
                plan.append("\002");
            }
            uint32_t planid = mid_plan_map_it->first;
            plan.append(boost::lexical_cast<string>(planid)).append("\003");
            MID_PLAN_DETAIL_MAP& mid_plan_detail_map = mid_plan_map_it->second;
            if (mid_plan_detail_map.find(SET_RATIO) != mid_plan_detail_map.end()) {
                plan.append(boost::lexical_cast<string>(mid_plan_detail_map[SET_RATIO].ratio)).append("\004");
                plan.append(boost::lexical_cast<string>(mid_plan_detail_map[SET_RATIO].crowdid));
            }
            plan.append("\003");
            if (mid_plan_detail_map.find(SET_RATIO_EXPAND) != mid_plan_detail_map.end()) {
                plan.append(boost::lexical_cast<string>(mid_plan_detail_map[SET_RATIO_EXPAND].ratio)).append("\004");
                plan.append(boost::lexical_cast<string>(mid_plan_detail_map[SET_RATIO_EXPAND].crowdid));
            }
            plan.append("\003");
            if (mid_plan_detail_map.find(SET_RATIO_ADID) != mid_plan_detail_map.end()) {
                adid.clear();
                trans_ad_2_str(mid_plan_detail_map[SET_RATIO_ADID].adid_ratio_map, adid);
                plan.append(adid);
            }
            plan.append("\003");
            if (mid_plan_detail_map.find(SET_RATIO_EXPAND_ADID) != mid_plan_detail_map.end()) {
                adid.clear();
                trans_ad_2_str(mid_plan_detail_map[SET_RATIO_EXPAND_ADID].adid_ratio_map, adid);
                plan.append(adid);
            }
            line.append(plan);
        }
        ofs << line << endl;
    }
}
bool deserialize_dict_txt(vector<dict_t>& dict_vec) {
    ifstream ins;
    string file_name = FLAGS_inputpath + "search_crowdid_planid_adid.txt";
    ins.open(file_name.c_str());
    if (ins.fail()) {
        cout << "search_crowdid_planid_adid file opening failed" << endl;
        exit(1);
    } else {
        cout << "search_crowdid_planid_adid opened successfully" << endl;
    }
    string line;
    vector<string> line_vec;
    while (getline(ins, line)) {
        line = searchengine::strip(line);
        if (line.empty()) continue;
        line_vec.clear();
        searchengine::split("", line, line_vec);
        if (line_vec.size() != 7) continue;
        dict_t dict;
        dict.crowdid = boost::lexical_cast<uint32_t>(line_vec[1]);
        dict.custid = boost::lexical_cast<uint32_t>(line_vec[2]);
        dict.planid = boost::lexical_cast<uint32_t>(line_vec[3]);
        dict.isExpand = boost::lexical_cast<uint32_t>(line_vec[4]);
        dict.ratio = boost::lexical_cast<float>(line_vec[5]);
        dict.adid = boost::lexical_cast<uint64_t>(line_vec[6]);
        dict_vec.push_back(dict);
    }
    cout << "dict_vec size: " << dict_vec.size() << endl;
    ins.close();
    return true;
}
bool deserialize_mid_txt(map<string,vector<uint32_t> >& mid_crowdid_map) {
    ifstream ins;
    string file_name = FLAGS_inputpath + "mid_crowdid.txt";
    ins.open(file_name.c_str());
    if (ins.fail()) {
        cout << "mid_crowdid file opening failed" << endl;
        exit(1);
    } else {
        cout << "mid_crowdid opened successfully" << endl;
    }
    string line;
    vector<string> mid_crowdid;
    vector<string> crowdidstr_vec;
    vector<uint32_t> crowdid_vec;
    while (getline(ins, line)) {
        line = searchengine::strip(line);
        if (line.empty()) continue;
        mid_crowdid.clear();
        crowdidstr_vec.clear();
        crowdid_vec.clear();
        searchengine::split("", line, mid_crowdid);
        if (mid_crowdid.size() != 2) continue;
        if (mid_crowdid[0].empty() || mid_crowdid[1].empty()) continue;
        searchengine::split("", mid_crowdid[1], crowdidstr_vec);
        if (mid_crowdid_map.find(mid_crowdid[0]) == mid_crowdid_map.end()) {
            mid_crowdid_map.insert(make_pair(mid_crowdid[0], crowdid_vec));
        }
        for (int i = 0; i < crowdidstr_vec.size(); ++i) {
            uint32_t crowdid = atoi(crowdidstr_vec[i].c_str());
            mid_crowdid_map[mid_crowdid[0]].push_back(crowdid);
        }
    }
    cout << "mid_crowdid_map size: " << mid_crowdid_map.size() << endl;
    ins.close();
}

void display_crowdid_map_info(map<uint32_t, DICT_PLAN_MAP>& crowdid_planid_map) { 
    cout << "---------------------The number of crowdid are: " << crowdid_planid_map.size() << "----------------" << endl;
    map<uint32_t, DICT_PLAN_MAP>::iterator it = crowdid_planid_map.begin();
    for (it; it != crowdid_planid_map.end(); it++) {
        cout << "Crowdid: "<< it->first <<endl;
        DICT_PLAN_MAP::iterator it2 = it->second.begin();
        for (it2; it2 != it->second.end(); it2++) {
            cout << "Planid: " << it2->first << endl;
            DICT_PLAN_DETAIL_MAP::iterator it3 = it2->second.begin();
            for (it3; it3 != it2->second.end(); it3++) {
                cout << "Case: " << it3->first << " ";
                if (it3->first == 0) {
                    cout << "Ratio is: " << it3->second.ratio<<endl;
                    cout << "isExpand is: " << it3->second.isExpand<<endl;
                }
                if (it3->first == 1) {
                    cout << "Ratio is: " << it3->second.ratio<<endl;
                    cout << "isExpand is: " << it3->second.isExpand<<endl;
                }
                if (it3->first == 2) {
                    map<uint64_t, float>::iterator it4 = it3->second.adid_ratio_map.begin();
                    for(it4; it4 != it3->second.adid_ratio_map.end();it4++) {
                        cout << "Adid: " << it4->first << " Ratio: " << it4->second <<endl;
                    }
                    cout << "isExpand is: " << it3->second.isExpand<<endl;
                }
                if (it3->first == 3) {
                    map<uint64_t,float>::iterator it4 = it3->second.adid_ratio_map.begin();

                    for(it4; it4 != it3->second.adid_ratio_map.end();it4++) {
                        cout << "Adid: " << it4->first << " Ratio: " << it4->second <<endl;
                    }
                    cout << "isExpand is: " << it3->second.isExpand<<endl;
                }
            }
            cout << endl;
        }
        cout << endl;
    }
}
void display_mid_map_info(MID_MAP& mid_map) {
    cout << "-------------------The number of mid: " << mid_map.size() << "----------------" << endl;
    MID_MAP::iterator it = mid_map.begin();
    for (it; it != mid_map.end(); it++) {
        cout << "Mid: " << it->first << endl;
        MID_PLAN_MAP::iterator it2 = it->second.begin();
        for (it2; it2 != it->second.end(); it2++) {
            cout << "Planid: " << it2->first<<endl;
            MID_PLAN_DETAIL_MAP::iterator it3 = it2->second.begin();
            for (it3; it3 != it2->second.end(); it3++) {
                cout << "Case: " << it3->first<<" ";
                if (it3->first == 0) {
                    cout << "Ratio is: "<< it3->second.ratio <<endl;
                    cout << "IsExpand is: " << it3->second.isExpand <<endl;
                    cout << "crowdid is: " <<it3->second.crowdid<<endl;
                }
                if (it3->first == 1) {
                    cout << "Ratio is: "<< it3->second.ratio <<endl;
                    cout << "IsExpand is: " << it3->second.isExpand <<endl;
                    cout << "crowdid is: " <<it3->second.crowdid<<endl;
                }
                if (it3->first == 2) {
                    map<uint64_t, pair<float,uint32_t> >::iterator it4 = it3->second.adid_ratio_map.begin();
                    for(it4; it4 != it3->second.adid_ratio_map.end(); it4++) {
                        cout << "Adid: " << it4->first << " Ratio: " << it4->second.first << " crowdid: " << it4->second.second<<endl;
                    }
                    cout << "isExpand is: " << it3->second.isExpand<<endl;
                }
                if (it3->first == 3) {
                    map<uint64_t, pair<float,uint32_t> >::iterator it4 = it3->second.adid_ratio_map.begin();
                    for(it4; it4 != it3->second.adid_ratio_map.end(); it4++) {
                        cout << "Adid: " << it4->first << " Ratio: " << it4->second.first << " crowdid: " << it4->second.second<<endl;
                    }
                    cout << "isExpand is: " << it3->second.isExpand<<endl;
                }
            }
            cout << endl;
        }
        cout <<endl;
    }
}
bool deserialize_baseline(map<uint32_t, vector<uint32_t> >& cust_plan_map, map<uint32_t, vector<uint32_t> >& plan_group_map) {
    ifstream ins;
    string file_name = FLAGS_inputpath + "ad.json";
    ins.open(file_name.c_str());
    if(ins.fail()){
        cout << "ad json file opening failed"<<endl;
        exit(1);
    } else{
        cout << "File " << file_name << " opened successfully"<<endl;
    }

    Json::Value value;
    Json::Reader reader;
    string line;
    while (getline(ins, line)) {
        bool is_parsed = reader.parse(line.c_str(),value);
        if (!is_parsed) {
            cout << "Failed to parse" << reader.getFormattedErrorMessages()<<endl;
            return false;
        }
        uint32_t group_id = value.get("groupId",0).asUInt();
        uint32_t plan_id = value.get("planId",0).asUInt();
        uint32_t account_id = value.get("userId",0).asUInt();
        if (cust_plan_map.find(account_id) == cust_plan_map.end()) {
            vector<uint32_t> v;
            cust_plan_map.insert(make_pair(account_id, v));
        }
        cust_plan_map[account_id].push_back(plan_id);

        if (plan_group_map.find(plan_id) == plan_group_map.end()) {
            vector<uint32_t> v;
            plan_group_map.insert(make_pair(plan_id, v));
        }
        plan_group_map[plan_id].push_back(group_id);
    }
    cout << "cust_plan_map size: " << cust_plan_map.size() <<endl;
    cout << "plan_group_map size: " << plan_group_map.size() <<endl;
    ins.close();
    return true;
}

int main(int argc,char** args) {
    google::ParseCommandLineFlags(&argc, &args, false);
    //account-planid-groupid
    map<uint32_t, vector<uint32_t> >* cust_plan_map = new map<uint32_t, vector<uint32_t> >();
    map<uint32_t, vector<uint32_t> >* plan_group_map = new map<uint32_t, vector<uint32_t> >();
    deserialize_baseline(*cust_plan_map, *plan_group_map);
    // mid-crowdid
    map<string, vector<uint32_t> >* mid_crowdid_map = new map<string,vector<uint32_t> >();
    deserialize_mid_txt(*mid_crowdid_map);
//    cout << "-----------------mid_map_size: " << mid_crowdid_map->size() << "--------------" << endl;
//    for (map<string, vector<uint32_t> >::iterator it = mid_crowdid_map->begin(); it != mid_crowdid_map->end(); ++it) {
//        cout << it->first << ": ";
//        for (int i = 0; i < it->second.size(); ++i) {
//            cout << it->second[i] << " ";
//        }
//        cout << endl;
//    }

    // crowdid-plan-detail
    vector<dict_t> dict_vec;
    deserialize_dict_txt(dict_vec);
    map<uint32_t, DICT_PLAN_MAP> crowdid_planid_map;
    update_crowdid_map(dict_vec, *cust_plan_map, crowdid_planid_map);
    //display_crowdid_map_info(crowdid_planid_map);

    MID_MAP* mid_map = new MID_MAP;
    update_mid_map(crowdid_planid_map, mid_crowdid_map, *mid_map);
    //display_mid_map_info(*mid_map);
    delete_useless_adid(*mid_map);
    //display_mid_map_info(*mid_map);
    serialize_mid(*mid_map);
    return 0;
}
