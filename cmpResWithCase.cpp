#include <iostream>
#include <fstream>
#include <vector>
#include <set>

using namespace std;
//所有的消费节点
int consumers[999];
vector<int>consumersVec;
//case的文件
ifstream input("/Users/sooglejay/MasterAndResume/华为比赛/HW-1.3/HUAWEI_Code_Craft_2017_Preliminary_Contest_Question_zh_v1.3/project/answer_demo/release/fixed_bug_edtion_4_2/cases2/2/case1.txt");
//结果文件
ifstream output("/Users/sooglejay/MasterAndResume/华为比赛/HW-1.3/HUAWEI_Code_Craft_2017_Preliminary_Contest_Question_zh_v1.3/project/answer_demo/release/fixed_bug_edtion_4_2/ou.txt");

//每一个消费节点  相关的供流路径
vector<vector<string>>supplyLinks(999);

set<int>serversList;

vector<string> split(const  string& s, const string& delim)
{
    vector<string> elems;
    size_t pos = 0;
    size_t len = s.length();
    size_t delim_len = delim.length();
    if (delim_len == 0) return elems;
    while (pos < len)
    {
        int find_pos = s.find(delim, pos);
        if (find_pos < 0)
        {
            elems.push_back(s.substr(pos, len - pos));
            break;
        }
        elems.push_back(s.substr(pos, find_pos - pos));
        pos = find_pos + delim_len;
    }
    return elems;
}

void getReq(){
    string line;
    int networkNodeCount,linkCount,consumerCount;
    input>>networkNodeCount;
    input>>linkCount;
    input>>consumerCount;

    while (linkCount+5) {
        getline(input, line);
        linkCount--;
    }
    while(getline(input,line)){
        vector<string>linVec = split(line," ");
        size_t len = linVec.size();
        int consumerNode = stoi(linVec[0]);
        int req = stoi(linVec[len-1]);

        //记录所有的消费节点的需求
        consumers[consumerNode] = req;

        //记录所有的消费节点
        consumersVec.push_back(consumerNode);
    }
}

void doSupply(){

    int linkCount;
    output>>linkCount;
    cout<<"您的链路一共"<<linkCount<<"条"<<endl;
    string line;
    //前两行不用管
    getline(output,line);
    getline(output,line);

    while (getline(output, line)) {
        vector<string>linVec = split(line," ");
        size_t len = linVec.size();
        int consumerNode = stoi(linVec[len-2]);
        int req = stoi(linVec[len-1]);
        int serverNode = stoi(linVec[0]);
        serversList.insert(serverNode);
        //serverNode 給它供流
        consumers[consumerNode] -=  req;
        supplyLinks[consumerNode].push_back(line);
    }
}
int doCheck(){
    bool hasError = false;
    for (int i = 0; i < consumersVec.size(); ++i) {
        int node = consumersVec[i];
        int remainFlow = consumers[node];
        if(remainFlow>0){
            hasError = true;
            cout<<"消费节点"<<consumersVec[i]<<"没有满足，还需"<<remainFlow<<endl;
            vector<string>links = supplyLinks[node];
            cout<<"給消费节点"<<node<<"供流的链路如下：\n";
            for (int j = 0; j < links.size(); ++j) {
                cout<<"第("<<(j+1)<<")条:"<<links[j]<<endl;
            }
        }
    }
    return hasError;
}
int main() {
    memset(consumers,0, sizeof(consumers));
    //获取原始需求
    getReq();
    //服务器供流
    doSupply();
    //检查是否有 消费节点没有满足
    if(doCheck()){
        cout<<"服务器如下：\n";

        for (set<int>::iterator ite = serversList.begin();ite!=serversList.end(); ite++) {
            cout<<*ite<<"  "<<endl;
        }
        cout<<"服务器个数："<<serversList.size()<<endl;
        cout<<"发现未满足的消费节点,请检查"<<endl;
    }else{
        cout<<"恭喜您！通过所有测试用例！"<<endl;
    }
    return 0;
}
