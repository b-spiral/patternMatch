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
	static	std::auto_ptr<NodeSet>	buildFromNodeList( std::map<int,int> *pMapNodeIndexToNodesetIndex, const std::vector<Node>& nodes)
	{
		std::auto_ptr<NodeSet>	pRet(new NodeSet);
		NodeSet& ns = *pRet;

		int	totalEntry = 0;
		for (int i = 0; i < nodes.size(); i++) {
			totalEntry += nodes[i].chrToNextIndex.size();
		}

		ns.chrIndexs.reserve(totalEntry);
		ns.nds.resize( nodes.size() );
		for (int i = 0; i < nodes.size(); i++) {
			const Node& node = nodes[i];
			Nd& nd = ns.nds[i];

			nd.chrIndexBegin = ns.chrIndexs.size();
			for (std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.begin(), itE = node.chrToNextIndex.end(); it != itE; it++) {
				ChrIndex	ci;
				ci.chr = it->first;
				ci.i = it->second;
				ns.chrIndexs.push_back(ci);
			}
			nd.chrIndexEnd = ns.chrIndexs.size();

			nd.depth = node.depth;
			nd.failIndex = node.failIndex;
			nd.matchIndex = node.matchIndex;
			nd.matchPatternno = node.matchPatternno;
		}

		if (pMapNodeIndexToNodesetIndex) {
			pMapNodeIndexToNodesetIndex->clear();
			for (int i = 0; i < nodes.size(); i++) {
				pMapNodeIndexToNodesetIndex->insert(std::make_pair(i, i));
			}
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
	
	struct ComparatorChrIndexChrLess
	{
		inline	bool	operator()(const ChrIndex& ci, const chr_t& chr)	const	{	return	ci.chr < chr;	}
	};

	struct Nd
	{
		int	chrIndexBegin, chrIndexEnd;
		int	matchPatternno;
		int	depth;
		int	failIndex;
		int	matchIndex;
	};
	std::vector<Nd>	nds;

public:
	std::pair<bool, int>	searchNext(int index, chr_t chr)	const
	{
		const Nd& nd = nds[index];
		std::vector<ChrIndex>::const_iterator itB = chrIndexs.begin();
		std::advance(itB, nd.chrIndexBegin);
		std::vector<ChrIndex>::const_iterator itE = chrIndexs.begin();
		std::advance(itE, nd.chrIndexEnd);
		std::vector<ChrIndex>::const_iterator it = std::lower_bound(itB, itE, chr, ComparatorChrIndexChrLess());
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
		const Nd& nd = nds[index];
		return	nd.depth;
	}
	int	getMatchPatternno(int index)	const
	{
		const Nd& nd = nds[index];
		return	nd.matchPatternno;
	}
};
