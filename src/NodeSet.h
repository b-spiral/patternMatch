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

		//�R���p�N�g�ȕ\���̂��߂Ɉȉ����s��
		//	Node.chrToNextIndex��map<chr_t,int>[]��񕪒T���\��vector< {chr_t,int} >��A���������̂ɒu��������B
		//	Node.depth��Nd�ɕێ����Ȃ��ōς܂��邽�߂ɁANd[ndIdx]�ɑΉ�����Node.depth��ndIdx�ɑ΂��ă\�[�g���ꂽ�`���ɂ���B

		//nodes[nodeIdx]��ns.nds[nsIdx]�Ƃ̑Ή��֌W�����߂�
		std::vector<int>	nodeIdxToNsIdx_body;
		if (!pNodeIndexToNodesetIndex) {
			pNodeIndexToNodesetIndex = &nodeIdxToNsIdx_body;
		}
		pNodeIndexToNodesetIndex->clear();
		std::vector<int>&	nodeIdxToNsIdx = *pNodeIndexToNodesetIndex;

		//nodes�̃C���f�b�N�X����Adepth�̏����ɕ��ׂ�B
		//	�C���f�b�N�X0�̓��[�g�Ƃ��ČŒ肷��̂Ń\�[�g�Ώۂɂ��Ȃ�
		std::vector<int>	nsIdxToNodeIdx(nodes.size(), 0 );
		for (int i = 0; i < nodes.size(); i++) {
			nsIdxToNodeIdx[i] = i;
		}
		std::sort( ++(nsIdxToNodeIdx.begin()), nsIdxToNodeIdx.end(), Less_NodeIdxDepth_NodeeIdxDepth(nodes) );

		nodeIdxToNsIdx.resize(nsIdxToNodeIdx.size());
		for (int nsIdx = 0; nsIdx < nodes.size(); nsIdx++) {
			nodeIdxToNsIdx[nsIdxToNodeIdx[nsIdx]] = nsIdx;
		}

		std::vector<int>&	depthToNsIdx = ns.depthToIdx;
		depthToNsIdx.clear();
		for (std::vector<int>::const_iterator itB = nsIdxToNodeIdx.begin(), it = itB, itE = nsIdxToNodeIdx.end(); it != itE; ) {
			int	depth = nodes[*it].depth;
			int	nsIdx = std::distance(itB, it);
			while (depthToNsIdx.size() <= depth) {
				depthToNsIdx.push_back(nsIdx);
			}

			it = std::upper_bound(it, itE, depth, Less_Depth_NodeIdxDepth(nodes));
		}

		//nsIdx����nodeIdx�Ƃ̑Ή���ϊ����Ȃ���Amap<chr_t,int>[]��vector< {chr_t,int} >�ɒǋL���Ă���

		{	//���ʂȍĔz�u���N����Ȃ��悤�ɕK�v�ʂ�reserve
			int	totalEntry = 0;
			for (int i = 0; i < nodes.size(); i++) {
				totalEntry += nodes[i].chrToNextIndex.size();
			}
			ns.chrIndexs.reserve(totalEntry);
		}

		ns.nds.resize( nodes.size() );
		for (int nsIdx = 0; nsIdx < nodes.size(); nsIdx++) {
			Nd& nd = ns.nds[nsIdx];
			const Node& node = nodes[ nsIdxToNodeIdx[nsIdx] ];

			nd.chrIndexBegin = ns.chrIndexs.size();
			for (std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.begin(), itE = node.chrToNextIndex.end(); it != itE; it++) {
				ChrIndex	ci;
				ci.chr = it->first;
				ci.i = nodeIdxToNsIdx[it->second];
				ns.chrIndexs.push_back(ci);
			}
			nd.chrIndexEnd = ns.chrIndexs.size();

			nd.failIndex = node.failIndex<0 ? -1 : nodeIdxToNsIdx[node.failIndex];
			nd.matchIndex = node.matchIndex<0 ? -1 : nodeIdxToNsIdx[node.matchIndex];
			nd.matchPatternno = node.matchPatternno;
		}

		return	pRet;
	}

private:
	NodeSet()
	{}

private:
	struct ChrIndex
	{
		chr_t	chr;
		int		i;
	};
	std::vector<ChrIndex>	chrIndexs;
	
	struct Less_ChrIdxChr_Chr
	{
		inline	bool	operator()(const ChrIndex& ci, chr_t chr)	const	{	return	ci.chr < chr;	}
	};
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
	struct Less_Depth_NodeIdxDepth
	{
		const std::vector<Node>& nodes;
		Less_Depth_NodeIdxDepth(const std::vector<Node>& nodes_)
			: nodes(nodes_) {}
		bool	operator()(int depth, int nodeIdx )	const
		{
			return	depth < nodes[nodeIdx].depth;
		}
	};
	struct Nd
	{
		int	chrIndexBegin, chrIndexEnd;
		int	matchPatternno;
		int	failIndex;
		int	matchIndex;
	};
	std::vector<Nd>	nds;
	std::vector<int>	depthToIdx;

public:
	std::pair<bool, int>	searchNext(int index, chr_t chr)	const
	{
		const Nd& nd = nds[index];
		std::vector<ChrIndex>::const_iterator itB = chrIndexs.begin();
		std::advance(itB, nd.chrIndexBegin);
		std::vector<ChrIndex>::const_iterator itE = chrIndexs.begin();
		std::advance(itE, nd.chrIndexEnd);
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
