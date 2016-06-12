#pragma	once
#include <vector>
#include <map>

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
		if (pMapNodeIndexToNodesetIndex) {
			pMapNodeIndexToNodesetIndex->clear();
			for (int i = 0; i < nodes.size(); i++) {
				pMapNodeIndexToNodesetIndex->insert(std::make_pair(i, i));
			}
		}
		return	std::auto_ptr<NodeSet>(new NodeSet(nodes));
	}

private:
	explicit	NodeSet(const std::vector<Node>& nodes_)
		: nodes(nodes_)
	{}

private:
	std::vector<Node>	nodes;

public:
	std::pair<bool, int>	searchNext(int index, chr_t chr)	const
	{
		const Node&	node = nodes[index];
		std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.find(chr);
		if (it != node.chrToNextIndex.end()) {
			return	std::make_pair(true, it->second);
		}
		else {
			return	std::make_pair(false, -1);
		}
	}
	int	getMatchIndex(int index)	const
	{
		const Node&	node = nodes[index];
		return	node.matchIndex;
	}
	int	getFailIndex(int index)	const
	{
		const Node&	node = nodes[index];
		return	node.failIndex;
	}
	int	getDepth(int index)	const
	{
		const Node&	node = nodes[index];
		return	node.depth;
	}
	int	getMatchPatternno(int index)	const
	{
		const Node&	node = nodes[index];
		return	node.matchPatternno;
	}
};
