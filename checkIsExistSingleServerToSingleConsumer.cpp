#include <iostream>
#include <fstream>
#include <vector>
#include <set>

using namespace std;


/**
 *
 * @param s 源字符串
 * @param delim 分隔符号  比如"\n"
 * @return
 */
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

/* 修正得到的链路信息中一个服务器节点只给一个消费节点供流的情况，使得消费节点直连的网络节点作为服务器
 * @prag const std::string& sLinkInfoInput：链路信息，每条链路以\n隔开
 * @prag int& iFinalLinkNumOutput：修正后的链路总数
 * @prag  std::string& sFinalLinkInfoOutput：修正后的链路信息，每条链路以\n隔开
 */
void fixFinalLinkInfo(const std::string& sLinkInfoInput, int& iFinalLinkNumOutput, std::string& sFinalLinkInfoOutput)
{
    vector<string> pathVec = split(sLinkInfoInput,"\n");
    vector<vector<string>>pathOfServers(1000);

    vector<string>finalOutputVec;
    for (int i = 0; i < pathVec.size(); ++i) {
        string path = pathVec[i];
        if(path.size()<1)continue;
        vector<string>linkVec = split(path," ");
        int serverNode = stoi(linkVec[0]);
        pathOfServers[serverNode].push_back(path);
    }
    for (int j = 0; j < pathOfServers.size(); ++j) {
        vector<string> pathsOfServerNode = pathOfServers[j];

        //服务器只给一个消费节点供流
        if(pathsOfServerNode.size()!=1)continue;

        string path = pathsOfServerNode[0];
        vector<string>linkVec = split(path," ");
        size_t sizeOfVec = linkVec.size();

        string req = linkVec[sizeOfVec-1];
        string consumerNode = linkVec[sizeOfVec-2];
        string consumerAdjNode = linkVec[sizeOfVec-3];

        string optPath = consumerAdjNode+" "+consumerNode+" "+req;
        finalOutputVec.push_back(optPath);
    }

    //若不存在这样的情况就 保持不变
    if(finalOutputVec.size()<1){
        sFinalLinkInfoOutput = sLinkInfoInput;
        return;
    }
    for (int l = 0; l < finalOutputVec.size(); ++l) {
        sFinalLinkInfoOutput += finalOutputVec[l]+"\n";
    }
    for (int j = 0; j < pathOfServers.size(); ++j) {
        vector<string> pathsOfServerNode = pathOfServers[j];
        //再获取其他的路径
        if(pathsOfServerNode.size()>1){
            for (int i = 0; i < pathsOfServerNode.size(); ++i) {
                sFinalLinkInfoOutput+=pathsOfServerNode[i]+"\n";
            }
        }
    }

}

int main() {

    string inpu = "9 2 3 4\n5 6 7 8\n9 10 11 21 13 14\n9 4 5\n\n\n\n\n5 23 34 45 56 5 67\n5 4 34 23 34\n";
    string out;
    int a  =12;
    fixFinalLinkInfo(inpu,a,out);
    cout<<out;
    return 0;
}
