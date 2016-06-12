#pragma	once
#include <vector>
#include <map>
#include <algorithm>

class NodeSet
{
public:
	typedef	int32_t	chr_t;

	struct Node
	{
		std::map<chr_t, int>	chrToNextIndex;
		int	matchPatternno;

		int	depth;
		int	failIndex;
		int	matchIndex;

		explicit	Node(int depth_)
			: depth(depth_), matchPatternno(-1)
		{}
	};

public:
	static	std::auto_ptr<NodeSet>	buildFromNodeList(const std::vector<Node>& nodes)
	{
		return	buildFromNodeList( nullptr, nodes );
	}

	static	std::auto_ptr<NodeSet>	buildFromNodeList(std::vector<int> *pNodeIndexToNodesetIndex, const std::vector<Node>& nodes)
	{
		std::auto_ptr<NodeSet>	pRet(new NodeSet);
		NodeSet& ns = *pRet;

		std::vector<int>	nodeIdxToNsIdx_body;
		if (!pNodeIndexToNodesetIndex) {
			pNodeIndexToNodesetIndex = &nodeIdxToNsIdx_body;
		}
		else {
			pNodeIndexToNodesetIndex->clear();
		}
		std::vector<int>&	nodeIdxToNsIdx = *pNodeIndexToNodesetIndex;

		//コンパクトな表現のために以下を行う
		//	Node.chrToNextIndexのmap<chr_t,int>[]を二分探索可能なvector< {chr_t,int} >を連結したものに置き換える。
		//	Node.depthをNdに保持しないで済ませるために、Nd[ndIdx]に対応するNode.depthはndIdxに対してソートされた形式にする。

		//nodes[nodeIdx]とns.nds[nsIdx]との対応関係を決める
		//	主にindex→depth検索用データのため
		std::vector<int>	nsIdxToNodeIdx;
		buildFromNodeList_makeNsIdxOrder( &nsIdxToNodeIdx, nodes );

		nodeIdxToNsIdx.resize(nsIdxToNodeIdx.size());
		for (int nsIdx = 0; nsIdx < nodes.size(); nsIdx++) {
			nodeIdxToNsIdx[nsIdxToNodeIdx[nsIdx]] = nsIdx;
		}

		//index→depth検索用データを作る
		ns.setupDepthList(nsIdxToNodeIdx, nodes);

		//nsIdx順にnodeIdxとの対応を変換しながら、map<chr_t,int>[]をvector< {chr_t,int} >に追記していく
		ns.setupNdList(nsIdxToNodeIdx, nodeIdxToNsIdx, nodes);

		return	pRet;
	}
private:
	static	void	buildFromNodeList_makeNsIdxOrder(std::vector<int> *pNsIdxToNodeIdx, const std::vector<Node>& nodes)
	{
		std::vector<int>& nsIdxToNodeIdx = *pNsIdxToNodeIdx;
		nsIdxToNodeIdx.resize(nodes.size(), 0);

		//nodesのインデックス列を、depthの昇順に並べる。
		//	インデックス0はルートとして固定するのでソート対象にしない
		for (int i = 0; i < nodes.size(); i++) {
			nsIdxToNodeIdx[i] = i;
		}

		struct Less_NodeIdxDepth_NodeeIdxDepth
		{
			const std::vector<Node>& nodes;
			Less_NodeIdxDepth_NodeeIdxDepth(const std::vector<Node>& nodes_)
				: nodes(nodes_) {}
			bool	operator()(int l, int r)	const
			{
				int	diff = nodes[l].depth - nodes[r].depth;
				if (diff != 0) {
					return	diff < 0;
				}
				return	l < r;
			}
		};
		std::sort(++(nsIdxToNodeIdx.begin()), nsIdxToNodeIdx.end(), Less_NodeIdxDepth_NodeeIdxDepth(nodes));
	}

private:
	NodeSet()
	{}

	void	setupDepthList(const std::vector<int>& nsIdxToNodeIdx, const std::vector<Node>& nodes)
	{
		struct Less_Depth_NodeIdxDepth
		{
			const std::vector<Node>& nodes;
			Less_Depth_NodeIdxDepth(const std::vector<Node>& nodes_)
				: nodes(nodes_) {}
			bool	operator()(int depth, int nodeIdx)	const
			{
				return	depth < nodes[nodeIdx].depth;
			}
		};

		std::vector<int>&	depthToNsIdx = depthToIdx;
		depthToNsIdx.clear();
		for (std::vector<int>::const_iterator itB = nsIdxToNodeIdx.begin(), it = itB, itE = nsIdxToNodeIdx.end(); it != itE; ) {
			int	depth = nodes[*it].depth;
			int	nsIdx = std::distance(itB, it);
			while (depthToNsIdx.size() <= depth) {
				depthToNsIdx.push_back(nsIdx);
			}

			it = std::upper_bound(it, itE, depth, Less_Depth_NodeIdxDepth(nodes));
		}

	}

	void	setupNdList(const std::vector<int>& nsIdxToNodeIdx, const std::vector<int>& nodeIdxToNsIdx, const std::vector<Node>& nodes)
	{
		{	//無駄な再配置が起こらないように必要量をreserve
			int	totalEntry = 0;
			for (int i = 0; i < nodes.size(); i++) {
				totalEntry += nodes[i].chrToNextIndex.size();
			}
			chrIndexs.reserve(totalEntry);
		}

		nds.resize(nodes.size());
		for (int nsIdx = 0; nsIdx < nodes.size(); nsIdx++) {
			Nd& nd = nds[nsIdx];
			const Node& node = nodes[nsIdxToNodeIdx[nsIdx]];

			nd.chrIndexBegin = chrIndexs.size();
			for (std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.begin(), itE = node.chrToNextIndex.end(); it != itE; it++) {
				ChrIndex	ci;
				ci.chr = it->first;
				ci.i = nodeIdxToNsIdx[it->second];
				chrIndexs.push_back(ci);
			}

			nd.failIndex = node.failIndex<0 ? -1 : nodeIdxToNsIdx[node.failIndex];
			nd.matchIndex = node.matchIndex<0 ? -1 : nodeIdxToNsIdx[node.matchIndex];
			nd.matchPatternno = node.matchPatternno;
		}
	}

private:
	struct ChrIndex
	{
		chr_t	chr;
		int		i;
	};
	std::vector<ChrIndex>	chrIndexs;
	
	struct Nd
	{
		int	chrIndexBegin;
		int	matchPatternno;
		int	failIndex;
		int	matchIndex;
	};
	std::vector<Nd>		nds;
	std::vector<int>	depthToIdx;

public:
	std::pair<bool, int>	searchNext(int index, chr_t chr)	const
	{
		const Nd& nd = nds[index];
		std::vector<ChrIndex>::const_iterator itB = chrIndexs.begin();
		std::advance(itB, nd.chrIndexBegin);
		std::vector<ChrIndex>::const_iterator itE;
		if (index + 1 < nds.size()) {
			itE = chrIndexs.begin();
			std::advance(itE, nds[index + 1].chrIndexBegin);
		}else{
			itE = chrIndexs.end();
		}

		struct Less_ChrIdxChr_Chr
		{
			inline	bool	operator()(const ChrIndex& ci, chr_t chr)	const { return	ci.chr < chr; }
		};
		std::vector<ChrIndex>::const_iterator it = std::lower_bound(itB, itE, chr, Less_ChrIdxChr_Chr());
		if ( it != itE && it->chr == chr) {
			return	std::make_pair(true, it->i);
		}else{
			return	std::make_pair(false, -1);
		}
	}
	int	getMatchIndex(int index)	const
	{
		const Nd& nd = nds[index];
		return	nd.matchIndex;
	}
	int	getFailIndex(int index)	const
	{
		const Nd& nd = nds[index];
		return	nd.failIndex;
	}
	int	getDepth(int index)	const
	{
		std::vector<int>::const_iterator itB = depthToIdx.begin(), 
			it = std::upper_bound(itB, depthToIdx.end(), index);
		return	std::distance(itB, it)-1;
	}
	int	getMatchPatternno(int index)	const
	{
		const Nd& nd = nds[index];
		return	nd.matchPatternno;
	}
};
