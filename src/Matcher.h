#pragma	once
#include <stdint.h>
#include <limits>
#include <map>
#include <vector>
#include <deque>
#include <ostream>
#include <cassert>

class Matcher
{
public:
	typedef	int32_t	chr_t;
	const static chr_t	EOS = std::numeric_limits<chr_t>::max();

private:
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
	const static int	INDEX_ROOT = 0;

public:
	class PatternDictionary
	{
	private:
		std::vector<Node>	nodes;

	public:
		PatternDictionary()
		{
			//index==0������Ă���
			nodes.push_back(Node(0));
		}
	public:
		// [it,it_e)��patternNo�̃p�^�[���Ƃ��ēo�^����B
		void	addPattern(std::vector<chr_t>::const_iterator itChr, std::vector<chr_t>::const_iterator itChrE, int patternNo)
		{
			assert(patternNo >= 0);
			assert(itChr != itChrE);

			int	index = 0;
			for (; itChr != itChrE; itChr++) {
				if (*itChr == EOS) {
					//�p�^�[���s��
					throw	std::exception();
				}
				Node&	pat = nodes[index];
				std::map<chr_t, int>::iterator	it = pat.chrToNextIndex.find(*itChr);
				if (it == pat.chrToNextIndex.end()) {
					int	next = nodes.size();
					pat.chrToNextIndex.insert(std::make_pair(*itChr, next));

					nodes.push_back(Node(pat.depth + 1));	//pat�������ɂȂ�ꍇ����
					index = next;
				}
				else {
					index = it->second;
				}
			}

			if (nodes[index].matchPatternno >= 0) {
				//�p�^�[���d��
				throw	std::exception();
			}
			nodes[index].matchPatternno = patternNo;
		}

		//	�o�^�����p�^�[���Q�Ƀ}�b�`����Matcher�����B
		std::auto_ptr<Matcher>	buildMatcher()
		{
			struct QueueItem
			{
				int	indexPrev;
				int	index;
				chr_t	chr;

				QueueItem(int index_, int indexPrev_, chr_t chr_)
					: index(index_), indexPrev(indexPrev_), chr(chr_)
				{}
			};

			std::deque< QueueItem >	queue;

			{
				Node& node = nodes[INDEX_ROOT];
				node.failIndex = -1;	//�ԕ�
				node.matchIndex = -1;

				for (std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.begin(), itE = node.chrToNextIndex.end(); it != itE; it++) {
					queue.push_back(QueueItem(it->second, 0, it->first));
				}
			}

			while (queue.size() > 0) {
				const QueueItem&	item = queue.front();
				Node& node = nodes[item.index];
				const Node& nodePrev = nodes[item.indexPrev];

				node.matchIndex = (node.matchPatternno >= 0) ? item.index : nodePrev.matchIndex;

				node.failIndex = INDEX_ROOT;

				if (node.matchPatternno<0
					//�}�b�`�p�^�[�������݂���
					&& item.indexPrev != INDEX_ROOT
					//���A�O�m�[�h�����[�g�łȂ�(=depth��1���傫��)
					//	�ꍇ�̂�fail�����[�g�ȊO�ɂȂ肤��
					) {
					int	f = nodePrev.failIndex;
					while (f >= 0) {
						auto& prev_chrTo = nodes[f].chrToNextIndex;
						auto it = prev_chrTo.find(item.chr);
						if (it != prev_chrTo.end()) {
							node.failIndex = it->second;
							break;
						}

						f = nodes[f].failIndex;
					}
				}

				for (std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.begin(), itE = node.chrToNextIndex.end(); it != itE; it++) {
					queue.push_back(QueueItem(it->second, item.index, it->first));
				}

				queue.pop_front();
			}
			nodes[INDEX_ROOT].failIndex = INDEX_ROOT;
			return	std::auto_ptr<Matcher>(new Matcher(nodes));
		}
	};

	struct MatchResult
	{
		int	size, patternNo;

		//size_������chr���A�o�^���ꂽ�p�^�[��patternNo_�Ƀ}�b�`����A��\�킷MatchResult���\�z����B
		MatchResult(int size_, int patternNo_)
			: size(size_), patternNo(patternNo_)
		{
			assert(patternNo >= 0);
		}

		//1chr���o�^���ꂽ�p�^�[���Ƀ}�b�`���Ȃ��A��\�킷MatchResult���\�z����B
		MatchResult()
			: size(1), patternNo(-1)
		{}

		bool	operator==(const MatchResult& rhs)	const
		{
			const MatchResult& lhs = *this;
			return	lhs.size == rhs.size && lhs.patternNo == rhs.patternNo;
		}
	};

private:
	class NodeSet
	{
	public:
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
	NodeSet	ns;

private:
	//	see. PatternDictionary::buildMatcher()
	explicit	Matcher(const std::vector<Node>& nodes_)
		: ns(nodes_)
	{}

public:
	// [itChr,itChrE)��chr_t���o�^����Ă���p�^�[����ŕ������A*pResult�ɏ������ށB
	//	�����́A�擪�����×~�ɍŒ��p�^�[����v�ōs���B
	void	matchWhole(std::vector<MatchResult> *pResults, std::vector<chr_t>::const_iterator itChr, std::vector<chr_t>::const_iterator itChrE)	const
	{
		std::vector<MatchResult>&	results = *pResults;
		results.clear();

		int	index = INDEX_ROOT;
		for (; itChr != itChrE; itChr++) {
			index = travaseNodeTri(&results, index, *itChr);
		}

		if (index != INDEX_ROOT) {
			//index�����[�g�łȂ�
			//���}�b�`�r��
			//���I�[�����邽�߂Ƀp�^�[���Ɋ܂܂�Ȃ�����EOS��ǂݍ��܂���index�����[�g�ɂ܂Ŗ߂�
			index = travaseNodeTri(&results, index, EOS);

			//�uEOS��ǂݔ�΂��v�������ɏo�͂���Ă���̂ŏ���
			results.pop_back();
		}
		assert(index == INDEX_ROOT);
	}

private:

	//	startIndex�m�[�h�̏�Ԃ�chr������ǂݍ���ŏ�ԑJ�ڂ�����̃m�[�hindex��Ԃ��B�J�ڒ��Ɋm�肵���}�b�`���ʂ�*pResult�ɒǋL����B
	int		travaseNodeTri(std::vector<MatchResult> *pResults, int startIndex, chr_t chr)	const
	{
		std::vector<MatchResult>&	results = *pResults;

		int	index = startIndex;
		while (true) {
			std::pair<bool, int>	next = ns.searchNext(index, chr);
			if ( next.first ) {
				//���ڕ����őJ�ڂł���
				//	�J�ڂ��Ď��̕����ɐi��
				return	next.second;
			}
			else {
				//���ڕ����őJ�ڂł��Ȃ�
				if (index == INDEX_ROOT) {
					//���[�g�Ȃ̂�1�����ǂݔ�΂����o�͂��Ď��̕����ɐi��
					results.push_back(MatchResult());
					return	INDEX_ROOT;
				}
				else {
					int	lastDepth;
					if (ns.getMatchIndex(index) >= INDEX_ROOT) {
						//�o�H��Ƀ}�b�`�����p�^�[����������

						//�p�^�[���o��
						results.push_back(MatchResult(ns.getDepth(ns.getMatchIndex(index)), ns.getMatchPatternno(ns.getMatchIndex(index))) );

						lastDepth = ns.getDepth(index) - ns.getDepth(ns.getMatchIndex(index));

					}
					else {
						//�o�H��Ƀ}�b�`�����p�^�[���͂Ȃ�����
						lastDepth = ns.getDepth(index);
					}

					//fail�J�ڂŒ��ڃm�[�h��depth���󂭂Ȃ� = �}�b�`���Ȃ�����������ǂݔ�΂��Ă���
					//�ǂݔ�΂������������o��
					int	skipnum = lastDepth - ns.getDepth(ns.getFailIndex(index) );
					for (int i = 0; i < skipnum; i++) {
						results.push_back(MatchResult());
					}

					//fail�J�ڂ��ē��������ł�����x����
					index = ns.getFailIndex(index);
				}
			}
		}
	}
};


inline	std::ostream&	operator<<(std::ostream& ostm, const Matcher::MatchResult& res)
{
	ostm << "{s:" << res.size;
	if (res.patternNo >= 0) {
		ostm << ", p:" << res.patternNo;
	}
	else {
		ostm << ", p:unmatch";
	}
	ostm << "}";
	return	ostm;
}
