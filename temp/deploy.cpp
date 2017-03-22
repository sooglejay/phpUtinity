#include "deploy.h"
#include <sstream>
#include <vector>
#include <stack>
#include <list>
#include <queue>
#include <map>
#include <set>
// 首先确定服务器的位置，然后，如果一个点已经确定是服务器了，就给那个点一个标记，只能出，不能入，也就是不能让其他服务器经过，否则我这个服务器就要抢生意。具体再想想
using namespace std;

// 网络节点的个数
#define  MAX_V  1000
#define  MAX_CONSUMER_V  1000


typedef pair<int, int> P;//first 保存最短距离，second保存顶点编号
// 用于表示边的结构体(终点、容量、费用、反向边)

struct Edge {
    int to, cap, cost, rev;
public:
    Edge(int to, int cap, int cost, int rev) {
        this->to = to;
        this->cap = cap;
        this->cost = cost;
        this->rev = rev;
    }
};

struct Consumer {
    int totalRequireFlow, hasFlow, connectedNetworkNode;//所需流量、已有流量、链接的网络节点编号
};


int rentPrice;//每台服务器的租用费用
int V;//顶点数

vector<vector<Edge>> Nets(MAX_V);//网络节点结构


//只是一个想法，还没有确定，目前是暂时不考虑下面这个  奇怪的数组，和想法
//vector<vector<Edge>> Nets_f(MAX_V);//这个是前面一个变量的辅助，指方向相反的边组成的新的图，我把它们抽出来，单独考虑

vector<Consumer> Cons(MAX_CONSUMER_V);//消费节点结构

//存储 每一个网络节点的出度,目的是为了在同级层级网络节点中确定服务器部署位置，确定该选用哪一个点:当然是出度最大
int outDegree[MAX_V];


//存储每一个消费节点  的所有同一级别的 网络节点链表
//500表示500级，假设服务器不可能离用户超过499条边数的距离
vector<int> bfsLevel1[MAX_CONSUMER_V][500];
//这个数组是上一个数组的辅助，目的是确定每一个网络节点 相对于 每一个消费节点的BFS层级，离得越近，层级越小
int bfsLevel2[MAX_CONSUMER_V][MAX_V];

int maxiBfsLevel[MAX_CONSUMER_V];//存储每一个消费节点的最大层级，公式：layer*y*w <= 服务器部署费用

//每台服务器的价格
int costPerServer = 0;

//网络节点数、网络节点的链路数、消费节点数
int networkNodeCount, networkLinkCount, consumerNodeCount;

//存储最终的结果
vector<string> resPathVec;


void addNetEdge(int from, int to, int cap, int cost) {
    Edge fromE = Edge(from, cap, cost, int(Nets[from].size() - 1));
    Edge toE = Edge(to, cap, cost, int(Nets[to].size()));
    Nets[from].push_back(toE);
    Nets[to].push_back(fromE);

    outDegree[from] += cap;
    outDegree[to] += cap;


    //待定
//    Edge toE_f = Edge(from, cap, cost, int(Nets[from].size()));
//    Edge fromE_f = Edge(to, 0, -cost, int(Nets[to].size()-1));
//
//    Nets_f[to].push_back(toE_f);
//    Nets_f[from].push_back(fromE_f);

}

void addConsumerEdge(int consumerNode, int networkNode, int flowRequire) {
    Cons[consumerNode].connectedNetworkNode = networkNode;
    Cons[consumerNode].hasFlow = 0;
    Cons[consumerNode].totalRequireFlow = flowRequire;
}


void printVecInt(vector<int> vec, int count = -1) {
    if (count < 0) {
        cout << "A Vec<int> info:" << endl;
    } else {
        cout << "number " << count << "-th Vec<int> info:" << endl;
    }
    for (int i = 0; i < vec.size(); ++i) {
        cout << i << ":" << vec[i] << endl;
    }
}

void printEdge(Edge e, int count = -1) {
    if (count < 0) {
        cout << "A Edge info:" << endl;
    } else {
        cout << "number " << count << "-th Edge info:" << endl;
    }
    cout << "\te.to:" << e.to << endl;
    cout << "\te.cap:" << e.cap << endl;
    cout << "\te.cost:" << e.cost << endl;

}

void printVecEdge(vector<Edge> vec, int count = -1) {
    if (count < 0) {
        cout << "A Vector<Edge> info:" << endl;
    } else {
        cout << "number " << count << "-th Vector<Edge> info:" << endl;
    }
    for (int i = 0; i < vec.size(); ++i) {
        printEdge(vec[i], i);
    }
}

void printVecVecEdge(vector<vector<Edge>> vecvec) {
    for (int i = 0; i < vecvec.size(); ++i) {
        printVecEdge(vecvec[i], i);
    }
}

void printVecVecInt(vector<vector<int>> vecvec) {
    for (int i = 0; i < vecvec.size(); ++i) {
        printVecInt(vecvec[i], i);
    }
}


/***
 * 给定一个网络节点，判断它是否跟消费节点 直接相连
 * @return 若是则 返回消费节点 编号；否则返回－1
 */

int ifConnectedWithConsumer(int networkNode) {
    for (int i = 0; i < consumerNodeCount; ++i) {
        if (Cons[i].connectedNetworkNode == networkNode)return i;
    }
    return -1;
}

/**
 * 给定一个 消费节点，返回直接跟它相连的网络节点
 * @param consumerNode
 * @return
 */
int getConnectedNetworkNode(int consumerNode) {
    return consumerNode < consumerNodeCount ? Cons[consumerNode].connectedNetworkNode : -1;
}


/***
 * 通过BFS确定从每一个消费节点出发   每一个网络节点的距离标号，也就是层级，离消费节点越近，层级越低
 */
void bfs_level() {

    //这个变量是为了测试 数据中，网络节点的最大层级，来判断未来开多大的数组
    int maximumLevel = INT_MIN;
    memset(bfsLevel2, -1, sizeof bfsLevel2);
    memset(maxiBfsLevel, 0, sizeof maxiBfsLevel);
    for (int j = 0; j < consumerNodeCount; ++j) {
        //为了计算距离消费节点j最远的服务器部署 的网络节点层级
        int minCostForLevel = INT_MAX, minCapForLevel = INT_MAX;
        int tempMaximumLevelOfConsumerJ = INT_MIN;
        queue<int> que;
        //直接跟用户相连 是属于第1级别
        //Cons[j].connectedNetworkNode是第j个消费节点的网络节点编号
        //消费节点j的第1层级中加入直接跟它相连的网络节点
        int connectedNetworkNode = Cons[j].connectedNetworkNode;
        bfsLevel1[j][1].push_back(connectedNetworkNode);

        //下面的bfsLevel2一个辅助设计，为了获取每一个网络节点相对于第j个消费节点 的层级
        bfsLevel2[j][connectedNetworkNode] = 1;
        //加入直接跟消费节点相连的网络节点，它属于第1层级
        que.push(connectedNetworkNode);
        while (!que.empty()) {
            int v = que.front();
            que.pop();
            //v的邻接链表
            for (int i = 0; i < Nets[v].size(); ++i) {
                Edge &e = Nets[v][i];
                int l = bfsLevel2[j][v] + 1;

                //如果e.to节点还没有访问过
                if (bfsLevel2[j][e.to] < 0) {
                    //网络节点e.to相对于第j个消费节点的层级
                    //队列的头元素v 的邻接节点e.to,它的层级要在v的基础上加1

                    bfsLevel2[j][e.to] = l;
                    //第j个消费节点的第（l）层 的链表中 添加网络节点e.to
                    bfsLevel1[j][l].push_back(e.to);
                    que.push(e.to);

                    //意图见本函数第一行
                    tempMaximumLevelOfConsumerJ = max(l, tempMaximumLevelOfConsumerJ);
                    maximumLevel = max(l, maximumLevel);
                    minCapForLevel = min(minCapForLevel, l);
                    minCostForLevel = min(minCostForLevel, e.cost);


                    if (costPerServer <= minCapForLevel * minCostForLevel * l) {
                        maxiBfsLevel[j] = l;
                        minCapForLevel = INT_MIN;//把其中任意一个数置为无穷小，不再进入这个if语句
                    }
                }
            }
            //如果服务器费用非常大，那么有可能 maxiBfsLevel[j]一直是零
            if (!maxiBfsLevel[j]) {
                maxiBfsLevel[j] = tempMaximumLevelOfConsumerJ;
            }
        }
    }

//    for (int k = 0; k < consumerNodeCount; ++k) {
//        cout << "第" << k << "个消费节点:" << endl;
//        for (int i = 0; i < networkNodeCount; ++i) {
//            cout << "\t网络节点" << i << " 的层级是：" << bfsLevel2[k][i] << endl;
//        }
//    }
//    cout << "maximumLevel=" << maximumLevel << endl;
}


/**
* 核心算法：最小费用流
* 求解从 s到t流量为f 的最小费用流,返回费用
* 如果没有流量为f的流，则返回-1

 注意： 对于网络流算法执行过程中的受影响的反向边，可以在算法结束之后，再去修复算法影响了的反向边，
* @param s
* @param t
* @param f
* @param minCostRoute  存储本次算法执行完毕后的路径和流量
* @return
*/
int minCost(int s, int t, int f, vector<vector<pair<int, int>>> &minCostRoute) {
    //若是直接相连，直接返回服务器的价格
    if (s == Cons[t].connectedNetworkNode)return costPerServer;
    //下面的 几个数据结构用于 最小费用流算法
    int h[MAX_V];//网络节点 顶点的势  用于优化查找最小费用算法
    int dist[MAX_V];//最短距离
    int prevv[MAX_V], preve[MAX_V];//最短路中的前驱节点和 对应的边
    memset(prevv, 0, sizeof prevv);
    memset(preve, 0, sizeof preve);
    int res = 0;
    fill(h, h + V, 0);//初始化 顶点的势，都是0，表示没有任何流动

    //如果还需要流量，继续循环执行
    while (f) {
        //使用Dijkstra算法更新 h
        priority_queue<P, vector<P>, greater<P>> que;
        fill(dist, dist + V, INT_MAX);
        dist[s] = 0;
        que.push(P(0, s));
        while (!que.empty()) {
            P p = que.top();
            que.pop();
            int v = p.second;//顶点编号
            if (dist[v] < p.first)continue;
            for (int i = 0; i < Nets[v].size(); i++) {
                Edge &e = Nets[v][i];
                if (e.cap > 0 && dist[e.to] > (dist[v] + e.cost + h[v] - h[e.to])) {
                    dist[e.to] = dist[v] + e.cost + h[v] - h[e.to];
                    prevv[e.to] = v;
                    preve[e.to] = i;
                    que.push(P(dist[e.to], e.to));
                }
            }
        }
        //如果不能再有任何流 流向消费节点
        if (dist[t] == INT_MAX) {
            //无法满足用户的需求
            return -1;
        }
        for (int j = 0; j < V; ++j) {
            h[j] += dist[j];
        }
        int d = f;
        for (int k = t; k != s; k = prevv[k]) {
            d = min(d, Nets[prevv[k]][preve[k]].cap);
        }
        f -= d;
        res += d * h[t];
        vector<pair<int, int>> newFlowVec;
        for (int l = t; l != s; l = prevv[l]) {
            Edge &e = Nets[prevv[l]][preve[l]];
            e.cap -= d;
            Nets[l][e.rev].cap += d;
            pair<int, int> p = make_pair(l, d);
            //注意是反着存储的，比如 t->e->d->f->c->a->s ,源是s，汇是t，并且第二个参数是d，也就是某一次算法计算出来的流过整个路径的最小流量
            newFlowVec.push_back(p);
        }
        minCostRoute.push_back(newFlowVec);
    }

    //反向弧路径修复
    for (int m = 0; m < minCostRoute.size(); ++m) {
        int v = t;
        for (int i = 0; i < minCostRoute[m].size(); ++i) {
            pair<int, int> p = minCostRoute[m][i];
            Nets[v][p.first].cap -= p.second;
            v = p.first;
        }
    }
    return res;

}


//你要完成的功能总入口
void deploy_server(char *topo[MAX_EDGE_NUM], int line_num, const char *filename) {


    // 需要输出的内容
    char *topo_file = (char *) "17\n\n0 8 0 20\n21 8 0 20\n9 11 1 13\n21 22 2 20\n23 22 2 8\n1 3 3 11\n24 3 3 17\n27 3 3 26\n24 3 3 10\n18 17 4 11\n1 19 5 26\n1 16 6 15\n15 13 7 13\n4 5 8 18\n2 25 9 15\n0 7 10 10\n23 24 11 23";




    // 直接调用输出文件的方法输出到指定文件中(ps请注意格式的正确性，如果有解，第一行只有一个数据；第二行为空；第三行开始才是具体的数据，数据之间用一个空格分隔开)
    write_result(topo_file, filename);

}


/***
 * 第一步：读数据
 * @param inputFileName
 */
void inputData(const char *inputFileName) {
    FILE *fin;
    fin = fopen(inputFileName, "rb");
    fscanf(fin, "%d", &networkNodeCount);
    fscanf(fin, "%d", &networkLinkCount);
    fscanf(fin, "%d", &consumerNodeCount);
    fscanf(fin, "%d", &costPerServer);


    //一共有linkCount条网络链路
    int startNode = 0, endNode = 0, totalFlow = 0, c = 0;
    for (int i = 0; i < networkLinkCount; ++i) {
        fscanf(fin, "%d", &startNode);
        fscanf(fin, "%d", &endNode);
        fscanf(fin, "%d", &totalFlow);
        fscanf(fin, "%d", &c);
        addNetEdge(startNode, endNode, totalFlow, c);
    }

    int consumerRequireFlow = 0, consumerNode = 0, networkNode = 0;
    //一共有consumeNodeCount个消费节点
    for (int j = 0; j < consumerNodeCount; ++j) {
        fscanf(fin, "%d", &consumerNode);
        fscanf(fin, "%d", &networkNode);
        fscanf(fin, "%d", &consumerRequireFlow);
        addConsumerEdge(consumerNode, networkNode, consumerRequireFlow);
    }
    fclose(fin);
}


/**
 * 数据输入完毕后，从这里进入，开始处理
 */
void calculate() {
    //确定每一个网络节点的 层级
    bfs_level();
    for (int i = 0; i < consumerNodeCount; ++i) {
        if (Cons[i].hasFlow >= Cons[i].totalRequireFlow) {
            //用户已经满足了
            continue;
        }
        int maxiLevelOfServer = maxiBfsLevel[i];
        pair<int, int> minCostPair;//存储最小代价的网络节点，first是最小费用，second最小费用的网络节点
        vector<vector<pair<int, int>>> minCostRoute;//存储最小费用流路径,用于路径修复
        minCostPair.first = costPerServer;
        minCostPair.second = Cons[i].connectedNetworkNode;

        while (maxiLevelOfServer) {
            //在同一级的所有网络节点标号
            vector<int> nodesLabelOnSameLevel = bfsLevel1[i][maxiLevelOfServer];
            for (int j = 0; j < nodesLabelOnSameLevel.size(); ++j) {
                //在消费节点i的maxiLevelOfServer级下的第j个网络节点，部署服务器
                int networkNode = nodesLabelOnSameLevel[j];
                minCostRoute.clear();
                //核心算法，求最小费用流
                int priceTemp = minCost(nodesLabelOnSameLevel[j], i, Cons[i].totalRequireFlow, minCostRoute);
                if (priceTemp > 0 && priceTemp < minCostPair.first) {
                    minCostPair.first = priceTemp;//最小费用
                    minCostPair.second = nodesLabelOnSameLevel[j];//最小费用的网络节点
                }
            }
            maxiLevelOfServer--;
        }

        //这次计算之后，要记录路径了
        minCost(minCostPair.second, i, Cons[i].totalRequireFlow, minCostRoute);

        //第i个用户已经满足需求
        Cons[i].hasFlow = Cons[i].totalRequireFlow;

        for (int k = 0; k < minCostRoute.size(); ++k) {
            vector<pair<int, int>> vP = minCostRoute[k];
            stringstream strs;
            int flow = 0;
            for (int j = 0; j < vP.size(); ++j) {
                if (j > 0) {
                    strs << " ";
                }
                strs << vP[j].first;
                flow += vP[j].second;
            }
            string p = strs.str();
            reverse(p.begin(), p.end());
            resPathVec.push_back(p);
        }
    }

}

/**
 * 构造 结果字符串
 * @return
 */
string generateOutput() {
    int n = resPathVec.size();
    stringstream strs;
    strs << n << "\n";
    for (int j = 0; j < n; ++j) {
        strs << "\n" << resPathVec[j];
    }
    string s = strs.str();
    return s;

}

/**
 * 删除最终的结果
 */
void output(const char *outputFileName) {
    // 需要输出的内容
    char *topo_file;
    FILE *fout = fopen(outputFileName, "wb");


    string temp_str = generateOutput();
    // 输出结果
    topo_file = (char *) temp_str.c_str();

    fclose(fout);

    // 直接调用输出文件的方法输出到指定文件中(ps请注意格式的正确性，如果有解，第一行只有一个数据；第二行为空；第三行开始才是具体的数据，数据之间用一个空格分隔开)
    write_result(topo_file, outputFileName);
}

/**
 * 主程序入口
 * @param inputFileName
 * @param outputFileName
 */
void deploy_server_ex(const char *const inputFileName, const char *const outputFileName) {


    inputData(inputFileName);

    calculate();

    output(outputFileName);
}





