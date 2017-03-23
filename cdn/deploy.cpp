#include "deploy.h"
#include <sstream>
#include <vector>
#include <stack>
#include <list>
#include <queue>
#include <map>
#include <set>
#include <climits>
#include <algorithm>
#include <cstdlib>
#include <cstring>

typedef pair<int, int> Pii;

//节点类型  消费节点、网络节点、服务器节点
#define FLAG_CONSUMER 1
#define FLAG_NETNODE -1

//这个点已经确定作为服务器节点了
#define FLAG_SERVER 0
//这个是 作为备选服务器的网络节点，意思就是这个点 不确定是不是服务器
#define FLAG_SERVER_Condidate 2

class cmp{
public:bool operator()(pair<int, int>&a,pair<int, int>&b){
		return a.first>b.first;
	}
};

// 用于表示边的结构体(终点、容量、费用、反向边) 尾点
struct Edge {
	int to, cap, cost, rev;
	int to_flag;//指向的节点的节点类型，消费节点为1，网络节点为-1,服务器节点为0
public:
	Edge(int to, int cap, int cost, int rev, int flag) {
		this->to = to;
		this->cap = cap;
		this->cost = cost;
		this->rev = rev;
		this->to_flag = flag;
	}
};

struct Consumer {
	int totalRequireFlow, hasFlow, connectedNetworkNode;//所需流量、已有流量、链接的网络节点编号
};



// 只是在输出的时候，根据实际情况自己 指定编号规则，但是代码里面全部都是统一编号
// 消费节点编号  从网络节点的个数 值 开始递增
// 消费节点之间的 最短路径数据结构
struct ConsumerToConsumer {
	//开始用户、结束用户、路径上的最少的流量
	int start, end, cost, minFlow;

	// start到end的路径，节点的编号 按照统一的编号规则
	// 0-networkNodesCount 是网络节点，
	// networkNodesCount－consumerNodeCount 是消费节点。方便查找。
	// 判断时只需要 判断是否大于networkNodeCount即可，link存储了跟消费节点直接相连的网络节点，一直到 距离该消费节点最近的消费节点的编号
	vector<int> link;

	ConsumerToConsumer(int start, int end, int cost, int flow) {
		this->start = start;
		this->end = end;
		this->cost = cost;
		this->minFlow = flow;
	};
};

struct FinalRes{
	int cost;
	int flow;
	float costPerFlow;
	FinalRes(int cost,int flow,float cpf){
		this->cost = cost;
		this->flow = flow;
		this->costPerFlow = cpf;

	}
	vector<int>link;
};

class cmpByCostPerFlow{
public:bool operator()(pair<float, FinalRes>&a,pair<float, FinalRes>&b){
		return a.first<b.first;
	};
};

/**
 * 打印所有的 消费节点  与跟它最近的消费节点，以及路径
 */
void testAllDijsktra(vector<ConsumerToConsumer> c2cVec, int networkNodeCount) {
	for (int l = 0; l < c2cVec.size(); ++l) {
		ConsumerToConsumer consumer1 = c2cVec[l];
		cout << (consumer1.start - networkNodeCount) << " 号消费节点的最近消费节点是 ";
		cout << (consumer1.end - networkNodeCount) << " 路径如下\n\t";
		vector<int> pa = consumer1.link;
		for (int i = 0; i < pa.size(); ++i) {
			cout << (pa[i] >= networkNodeCount ? (pa[i] - networkNodeCount) : pa[i]);
			if (i < pa.size() - 1) {
				cout << " -> ";
			}
		}
		cout << "最小容量是" << consumer1.minFlow << endl;
		cout << endl << endl;
	}
}



/***
 *
 *  s 源消费节点,编号规则 与网络节点 统一
 *  consumer 消费节点 跟所有 其他消费节点中距离最短的消费节点，组成的一个结构体，包括两者之间的路径和费用
 */
void DijKstra(int s, ConsumerToConsumer &consumer, vector<vector<Edge> > &Nets, vector<Consumer> &Cons, int netsNodeCount,
			  int consumerNodeCount,bool* consumerDeleted) {

	//包括消费节点在内，图上的所有点数
	int totalCount = netsNodeCount + consumerNodeCount;
	//最短距离的路径
	vector<int> link;

	//存储s 到其他所有点的最短路径
	int *d=(int *)malloc(sizeof(int*)*totalCount);
	//松弛操作d[j]=d[k]+cost[k][j]时，修改prev[j]=k,k也就是j的直接前驱，s->j的最短路径是s->k->j
	int *prev=(int *)malloc(sizeof(int*)*totalCount);
	fill(d, d + totalCount, INT_MAX);
	fill(prev, prev + totalCount, -1);
	//消费节点 s到它自己的距离为0
	d[s] = 0;
	//消费节点 s到直接与它相连的网络节点距离也是0
	int netNodeOfConsumer = Cons[s - netsNodeCount].connectedNetworkNode;
	d[netNodeOfConsumer] = 0;

	priority_queue<Pii, vector<Pii>, greater<Pii>> que;

	//从跟s消费节点直接相连的网络节点开始BFS
	que.push(Pii(0, netNodeOfConsumer));

	while (!que.empty()) {
		Pii p = que.top();
		que.pop();
		int v = p.second;
		if (d[v] < p.first)continue;//d[v]不为无穷大，说明访问过了
		if (v >= netsNodeCount) {//到达一个消费节点 就continue，继续下一个
			continue;
		}
		for (int i = 0; i < Nets[v].size(); ++i) {
			Edge &e = Nets[v][i];
			//如果e是网络节点跟消费节点相连的边
			//判断e.to是否为除了s之外的消费节点， 是的话，我们就找到了其中一个
			if (e.to_flag == FLAG_CONSUMER) {
				if (e.to == s) {//如果是原来的消费节点s，就跳过
					continue;
				} else {
					// 网络节点v 直接跟消费节点e.to相连
					// 那么s到e.to的距离是s到v的距离
					d[e.to] = d[v];
					prev[e.to] = v;
					continue;
				}
			}
			if (d[e.to] > d[v] + e.cost) {
				d[e.to] = d[v] + e.cost;
				que.push(Pii(d[e.to], e.to));
				prev[e.to] = v;
			}
		}
	}

	//d 数组 就是 消费节点s 到其他全部节点的 最短路径值，然后从netsNodeCount开始找，（因为它是消费节点开始编号的值）找到数组d[k]最小的的k，k>=netsNodeCount

	//first 是费用，second是节点编号
	pair<int, int> pp = make_pair(INT_MAX, -1);

	//查找除了s之外的其他消费节点，谁距离s最近

	for (int k = netsNodeCount; k < totalCount; ++k) {

		if (k == s) {
			continue;
		}

		//如果这个消费节点已经满足了，就不要考虑它了
		if(Cons[k-netsNodeCount].hasFlow>=Cons[k-netsNodeCount].totalRequireFlow)continue;

		//如果此消费节点 已经被预优化了，也不考虑
		if(consumerDeleted[k-netsNodeCount])continue;

		if (d[k] < pp.first) {
			pp.second = k;
			pp.first = d[k];
		}
	}
	vector<int> path;
	int t = pp.second;
	//最短路径上的最小的流量，最小流量 乘以 本身就是最便宜的路径，先给用户再说。
	int minFlow = INT_MAX;
	int cc = 0;
	while (t != -1) {
		path.push_back(t);
		int k = prev[t];
		if (cc > 0&&k!=-1) {
			for (int i = 0; i < Nets[k].size(); ++i) {
				Edge &e = Nets[k][i];
				if (e.to == t) {
					minFlow = min(minFlow, e.cap);
					break;
				}
			}
		}
		t = k;
		cc++;
	}
	reverse(path.begin(), path.end());

	//构造数据结构，存储包括最短路径的 一个节点对
	consumer.start = s;
	consumer.end = pp.second;
	consumer.cost = pp.first;
	consumer.minFlow = minFlow;
	consumer.link = path;

	free(prev);
	free(d);
}



/**
 * 核心算法：最小费用流
 * 求解从 s到t流量为f 的最小费用流,返回费用
 * 如果没有流量为f的流，则返回-1
 */
int minCost(int s, int t, int f,int totalNodeCount,int networkNodeCount,vector<vector<Edge>>&Nets,vector<Consumer>&Cons,priority_queue<pair<float, FinalRes>,vector<pair<float, FinalRes>>,cmpByCostPerFlow>&pathVec,int*nodeType,int costPerServer) {

	priority_queue<pair<float, FinalRes>,vector<pair<float, FinalRes>>,cmpByCostPerFlow> tempPathQue;

	//所有链路的费用
	int totalCostOfLinks = 0;
	int flowOriginal = f;//先存着。等下面要用

	//下面的 几个数据结构用于 最小费用流算法
	//网络节点 顶点的势  用于优化查找最小费用算法
	int *h=(int *)malloc(sizeof(int)*totalNodeCount);

	//最短距离
	int *dist=(int *)malloc(sizeof(int)*totalNodeCount);

	//最短路中的前驱节点和 对应的边
	int *prevv=(int *)malloc(sizeof(int)*totalNodeCount);
	int *preve=(int *)malloc(sizeof(int)*totalNodeCount);
	//最终返回的不再是费用，而是用户剩下还需要的流量
	//    int res = 0;
	fill(h, h + totalNodeCount, 0);//初始化 顶点的势，都是0，表示没有任何流动

	while (f>0) {
		//使用Dijkstra算法更新 h
		priority_queue<Pii, vector<Pii>, greater<Pii>> que;
		fill(dist, dist+totalNodeCount, INT_MAX);
		dist[s] = 0;
		que.push(Pii(0, s));
		while (!que.empty()) {
			Pii p = que.top();
			que.pop();
			int v = p.second;//顶点编号
			if (dist[v] < p.first)continue;
			for (int i = 0; i < Nets[v].size(); i++) {
				Edge &e = Nets[v][i];
				if(nodeType[e.to]==FLAG_SERVER)continue;
				if (e.cap > 0 && dist[e.to] > (dist[v] + e.cost + h[v] - h[e.to])) {
					dist[e.to] = dist[v] + e.cost + h[v] - h[e.to];
					prevv[e.to] = v;
					preve[e.to] = i;
					que.push(Pii(dist[e.to], e.to));
				}
			}
		}
		//如果不能再有任何流 流向消费节点
		if (dist[t] == INT_MAX) {
			//无法满足用户的需求
			if(totalCostOfLinks>=costPerServer){
				//说明费用比单台服务器贵，就返回-1
				return -1;
			}//否则就返回用户剩下需要的流量
			if(f==flowOriginal){
				//说明一丁点儿流量都无法供应
				return -1;
			}
			//把路径记录下来
			while (!tempPathQue.empty()) {
				pathVec.push(tempPathQue.top());
				tempPathQue.pop();
			}
			return f;
		}
		for (int j = 0; j < totalNodeCount; ++j) {
			h[j] += dist[j];
		}
		int d = f;
		for (int k = t; k != s; k = prevv[k]) {
			d = min(d, Nets[prevv[k]][preve[k]].cap);
		}
		f -= d;

		int costSumOfLink=0;
		vector<int>link;

		//这里的减法表示我在添加s->t的路径的时候，也就是说，服务器->消费节点,我把消费节点编号修复了
		link.push_back(t-networkNodeCount);

		float maxFlowDivideByCost = 1000000;
		for (int l = t; l != s; l = prevv[l]) {
			Edge &e = Nets[prevv[l]][preve[l]];
			if(l!=t)
				link.push_back(l);
			e.cap -= d;
			maxFlowDivideByCost =min(maxFlowDivideByCost,e.cap*1.0F/e.cost);
			costSumOfLink+=d*(e.cost);
			Nets[l][e.rev].cap += d;
		}
		totalCostOfLinks += costSumOfLink;
		float cpf= costSumOfLink*1.0F/d;
		FinalRes fR(costSumOfLink,d,cpf);
		link.push_back(s);
		reverse(link.begin(), link.end());
		fR.link = link;
		tempPathQue.push(make_pair(costSumOfLink,fR));
	}

	free(preve);
	free(prevv);
	free(dist);
	free(h);
	if(totalCostOfLinks>=costPerServer){
		//说明费用比单台服务器贵，就返回-1
		return -1;
	}//否则就返回用户剩下需要的流量
	if(f==flowOriginal){
		//说明一丁点儿流量都无法供应
		return -1;
	}
	//把路径记录下来
	while (!tempPathQue.empty()) {
		pathVec.push(tempPathQue.top());
		tempPathQue.pop();
	}
	//如果返回0，说明满足用户的全部需求
	return 0;

}




/**
 * 返回服务器的位置
 * @param c2cVec  存储了所有消费节点和距离其最近的消费节点 ，以及路径
 * @param totalNodes  网络中所有的节点个数，包括消费节点
 * @param networkNodeCount 网络节点的个数
 * @return 服务器位置
 */
int findServerCandidateLocation(vector<ConsumerToConsumer> &c2cVec, int totalNodes, int networkNodeCount,int *nodeType,set<int>&AcNode) {

	if(c2cVec.size()==0)return -1;
	//下面开始找服务器位置
	//1、首先找出现频率最高的消费节点index
	int *arr=(int *)malloc(sizeof(int)*totalNodes);
	fill(arr, arr + totalNodes, 0);
	for (int l = 0; l < c2cVec.size(); ++l) {
		arr[c2cVec[l].end]++;
		arr[c2cVec[l].start]++;
	}
	int mostFrequency = INT_MIN, coinCidencePoint = 0;
	for (int pk = 0; pk < totalNodes; ++pk) {
		if (mostFrequency < arr[pk]) {
			mostFrequency = arr[pk];
			coinCidencePoint = pk;//频率最高
		}
	}


	//找到了！然后 根据index消费节点，找与之相对的m对消费节点
	vector<vector<int>> mPaths;
	for (int n = 0; n < c2cVec.size(); ++n) {
		//这里使用边的end 来作为条件判断，即最短的消费节点是 那个出现频率最高的消费节点，那么这条链路就是m之一
		if (c2cVec[n].end == coinCidencePoint) {
			mPaths.push_back(c2cVec[n].link);
		}
	}

	//继续使用这个数组，找上面m条链路的重合点,作为服务器的位置
	fill(arr, arr + totalNodes, 0);
	for (int i1 = 0; i1 < mPaths.size(); ++i1) {
		vector<int> link = mPaths[i1];
		//因为link 中，最后一个元素是 消费节点，它不用考虑了！
		for (int i = 0; i < link.size() - 1; ++i) {
			arr[link[i]]++;
		}
	}
	//继续使用这个变量
	mostFrequency = INT_MIN;
	//存储最终的重合点，也就是服务器的位置！服务器的位置
	int serverTarget = -1;
	for (int j1 = 0; j1 < totalNodes; ++j1) {
		if (mostFrequency < arr[j1]) {
			mostFrequency = arr[j1];
			serverTarget = j1;
		}
	}

	//删除这m对消费节点
	for (vector<ConsumerToConsumer>::iterator iter = c2cVec.begin(); iter != c2cVec.end();) {
		ConsumerToConsumer &consumer = *iter;
		if (consumer.end == coinCidencePoint || consumer.start == coinCidencePoint)
			iter = c2cVec.erase(iter);
		else
			iter++;
	}

	free(arr);
	return serverTarget;
}

/**
 读取边信息
 **/
void inputData(vector<vector<Edge>>&Nets,vector<Consumer>&Cons,vector<Pii>&outDegree,int *nodeType,int networkLinkCount,int consumerNodeCount,int networkNodeCount,int &totalFlowRequire,int **costMatrix,FILE *fin){
	//一共有linkCount条网络链路
	int from = 0, to = 0, cap = 0, cost = 0;
	for (int i = 0; i < networkLinkCount; ++i) {
		fscanf(fin, "%d", &from);
		fscanf(fin, "%d", &to);
		fscanf(fin, "%d", &cap);
		fscanf(fin, "%d", &cost);

		Edge toE = Edge(to, cap, cost, int(Nets[to].size()), FLAG_NETNODE);
		Nets[from].push_back(toE);

		Edge fromE = Edge(from, cap, cost, int(Nets[from].size() - 1), FLAG_NETNODE);
		Nets[to].push_back(fromE);


		//统计出度 first 是出度  second 是节点标号
		outDegree[from].first+=cap;
		outDegree[from].second=from;

		outDegree[to].first+=cap;
		outDegree[to].second=to;

		costMatrix[from][to] = cost;
		costMatrix[to][from]=cost;

		//记录节点类型
		nodeType[from] = nodeType[to] = FLAG_NETNODE;

	}

	int consumerRequireFlow = 0, consumerNode = 0, networkNode = 0;
	//一共有consumeNodeCount个消费节点
	for (int j = 0; j < consumerNodeCount; ++j) {
		fscanf(fin, "%d", &consumerNode);
		fscanf(fin, "%d", &networkNode);
		fscanf(fin, "%d", &consumerRequireFlow);

		Cons[consumerNode].connectedNetworkNode = networkNode;
		Cons[consumerNode].hasFlow = 0;
		Cons[consumerNode].totalRequireFlow = consumerRequireFlow;

		//消费节点 指向 网络节点的边，我把它还是归为网络边，只有网络节点指向消费节点，才是消费边
		Edge toE = Edge(networkNode, INT_MAX, 0, int(Nets[networkNode].size()), FLAG_NETNODE);
		//消费节点 -> 网络节点  边
		Nets[consumerNode + networkNodeCount].push_back(toE);

		//消费节点序号是直接从网络节点个数递增
		Edge fromE = Edge(consumerNode + networkNodeCount, INT_MAX, 0,
						  int(Nets[networkNodeCount + consumerNode].size()) - 1,
						  FLAG_CONSUMER);
		//网络节点  ->  消费节点 边
		Nets[networkNode].push_back(fromE);

		//记录所有消费节点的需求
		totalFlowRequire+=consumerRequireFlow;

		nodeType[consumerNode + networkNodeCount] = FLAG_CONSUMER;

	}


}
/***
 第一阶段：预优化 ，判断是否只能 把服务器直接部署到消费节点 的相邻网络节点
 ***/
void pre(vector<vector<Edge>>Nets,vector<Consumer>Cons,int consumerNodeCount,int &totalFlowRequire,vector<string>&resPathVec,vector<int>&serverList,int *nodeType,bool *consumerDeleted,int costPerServer){
	for (int conIndex = 0;conIndex<consumerNodeCount;conIndex++) {
		Consumer &consumer = Cons[conIndex];
		int netNode = consumer.connectedNetworkNode;
		int totalFlow = 0;

		//first 边的流量租用费用，second容量
		priority_queue<Pii, vector<Pii>, greater<Pii>> que;
		for (int i = 0; i < Nets[netNode].size(); ++i) {
			//必须加下面这个判断，否则就全盘皆输，因为会把消费节点也算进去，而它是未知数
			if(Nets[netNode][i].to_flag==FLAG_CONSUMER){
				continue;
			}
			totalFlow += Nets[netNode][i].cap;
			que.push(make_pair(Nets[netNode][i].cost, Nets[netNode][i].cap));
		}

		if (totalFlow < consumer.totalRequireFlow) {
			stringstream str;
			str << (resPathVec.size())<<" "<<netNode << " " << conIndex << " " << consumer.totalRequireFlow;
			resPathVec.push_back(str.str());
			consumer.hasFlow = consumer.totalRequireFlow;
			totalFlowRequire-=consumer.totalRequireFlow;
			serverList.push_back(netNode);
			nodeType[netNode]=FLAG_SERVER;
			consumerDeleted[conIndex] = true;

		} else {
			//如果 与消费节点直接相连的网络节点 所链接的链路最小流量花费都超过服务器租用费。就部署服务器
			int minCost = 0, reqLeft = consumer.totalRequireFlow;
			while (!que.empty()) {
				Pii p = que.top();que.pop();
				if (reqLeft > p.second) {
					reqLeft -= p.second;
					minCost += p.first * p.second;
				} else {
					minCost += p.first * reqLeft;
					reqLeft = 0;
				}
			}
			if (minCost >= costPerServer) {
				stringstream str;
				str <<  (resPathVec.size())<<" "<<netNode << " " << conIndex << " " << consumer.totalRequireFlow;
				resPathVec.push_back(str.str());
				consumer.hasFlow=consumer.totalRequireFlow;
				totalFlowRequire-=consumer.totalRequireFlow;
				serverList.push_back(netNode);
				nodeType[netNode]=FLAG_SERVER;
				consumerDeleted[conIndex] = true;

			}
		}
	}

}
/**
 第二阶段 找服务器位置
 ***/
void findServerList(vector<vector<Edge>>&Nets,vector<Consumer>&Cons,bool *consumerDeleted,int *nodeType,int networkNodeCount,int consumerNodeCount,int totalFlowRequire,vector<int>&serverCandidate,set<int>&AcNode,vector<Pii>&outDegree){

	//服务器候选节点  第一级开始
	//候选方法 是 取频率最高的路径重合点、出度最大优先节点
	vector<ConsumerToConsumer> c2cVec, c2cVecBackup;

	for (int k = 0; k < consumerNodeCount; ++k) {
		if(consumerDeleted[k]){
			continue;
		}
		if(Cons[k].hasFlow>=Cons[k].totalRequireFlow)continue;
		ConsumerToConsumer consumer(0, 0, 0, INT_MAX);
		//实现核心算法1 找到 任意一个消费节点 的最近的消费节点，组成一个节点对结构
		DijKstra(k + networkNodeCount, consumer, Nets, Cons, networkNodeCount, consumerNodeCount,consumerDeleted);
		c2cVec.push_back(consumer);
		c2cVecBackup.push_back(consumer);
	}


	testAllDijsktra(c2cVec,networkNodeCount);


	while (totalFlowRequire>0) {
		//实现核心算法2
		int res = findServerCandidateLocation(c2cVec, networkNodeCount+consumerNodeCount, networkNodeCount,nodeType,AcNode);
		if (res == -1) {
			break;
		}
		nodeType[res] = FLAG_SERVER_Condidate;
		serverCandidate.push_back(res);
	}


}

/**
 * 主程序入口
 */
void deploy_server_ex(const char *const inputFileName, const char * const outputFileName) {

	FILE *fin;
	fin = fopen(inputFileName, "rb");
	//存储最终的结果
	vector<string> resPathVec;
	//每台服务器的价格
	int costPerServer = 0;
	//网络节点数、网络节点的链路数、消费节点数
	int networkNodeCount, networkLinkCount, consumerNodeCount;
	fscanf(fin, "%d", &networkNodeCount);
	fscanf(fin, "%d", &networkLinkCount);
	fscanf(fin, "%d", &consumerNodeCount);
	fscanf(fin, "%d", &costPerServer);
	int totalNodes = networkNodeCount + consumerNodeCount;
	//记录每一个节点的类型
	int *nodeType = (int*)malloc(sizeof(int)*totalNodes);
	memset(nodeType, sizeof(nodeType), 0);
	//统计出度初始化  first是出度总和 ，second是节点编号
	vector<Pii>outDegree;
	for (int i=0; i<totalNodes; i++) {
		//出度初始化为0
		Pii p = make_pair(0, i);
		outDegree.push_back(p);
	}

	//所有消费节点的 总带宽需求
	int totalFlowRequire=0;

	//价格矩阵 初始化
	int **costMatrix = (int **)malloc(sizeof(int*)*networkNodeCount);
	for (int i=0; i<networkNodeCount; i++) {
		costMatrix[i]=(int *)malloc(sizeof(int)*networkNodeCount);
	}
	for (int i=0; i<networkNodeCount; i++) {
		for (int j=0; j<networkNodeCount; j++) {
			costMatrix[i][j] = 0;
		}
	}
	// 我这次打算把消费节点跟网络节点都存储在一个编号系统中，消费节点直接从MAX_V开始递增编号
	vector<vector<Edge>> Nets(totalNodes);
	vector<Consumer> Cons(consumerNodeCount);//消费节点结构

	//是否被预优化掉了
	bool* consumerDeleted=(bool*)malloc(sizeof(bool)*consumerNodeCount);
	bool* networkNodeDelete=(bool*)malloc(sizeof(bool)*networkNodeCount);
	memset(consumerDeleted, sizeof(consumerDeleted), false);
	memset(networkNodeDelete, sizeof(networkNodeDelete), false);

	//最终输出 使用的输出缓冲
	stringstream finalOutPutForAllConsumers;

	//服务器集合
	vector<int> serverList;
	//服务器候选集合
	vector<int>serverCandidate;
	//孤立节点
	set<int>AcNode;
	//读入边信息
	inputData(Nets, Cons, outDegree, nodeType, networkLinkCount, consumerNodeCount, networkNodeCount, totalFlowRequire, costMatrix, fin);


	//第一阶段，预优化
	pre(Nets, Cons, consumerNodeCount, totalFlowRequire, resPathVec, serverList, nodeType, consumerDeleted, costPerServer);

	//第二阶段，找服务器位置
	findServerList(Nets, Cons, consumerDeleted, nodeType, networkNodeCount, consumerNodeCount, totalFlowRequire, serverCandidate, AcNode,outDegree);


	for (int i=0; i<serverCandidate.size(); i++) {
		cout<<serverCandidate[i]<<endl;
	}
	write_result("", outputFileName);

}





