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
			//index==0を作っておく
			nodes.push_back(Node(0));
		}
	public:
		// [it,it_e)をpatternNoのパターンとして登録する。
		void	addPattern(std::vector<chr_t>::const_iterator itChr, std::vector<chr_t>::const_iterator itChrE, int patternNo)
		{
			assert(patternNo >= 0);
			assert(itChr != itChrE);

			int	index = 0;
			for (; itChr != itChrE; itChr++) {
				if (*itChr == EOS) {
					//パターン不正
					throw	std::exception();
				}
				Node&	pat = nodes[index];
				std::map<chr_t, int>::iterator	it = pat.chrToNextIndex.find(*itChr);
				if (it == pat.chrToNextIndex.end()) {
					int	next = nodes.size();
					pat.chrToNextIndex.insert(std::make_pair(*itChr, next));

					nodes.push_back(Node(pat.depth + 1));	//patが無効になる場合あり
					index = next;
				}
				else {
					index = it->second;
				}
			}

			if (nodes[index].matchPatternno >= 0) {
				//パターン重複
				throw	std::exception();
			}
			nodes[index].matchPatternno = patternNo;
		}

		//	登録したパターン群にマッチするMatcherを作る。
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
				node.failIndex = -1;	//番兵
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
					//マッチパターンが存在せず
					&& item.indexPrev != INDEX_ROOT
					//かつ、前ノードがルートでない(=depthが1より大きい)
					//	場合のみfailがルート以外になりうる
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

		//size_文字のchrが、登録されたパターンpatternNo_にマッチする、を表わすMatchResultを構築する。
		MatchResult(int size_, int patternNo_)
			: size(size_), patternNo(patternNo_)
		{
			assert(patternNo >= 0);
		}

		//1chrが登録されたパターンにマッチしない、を表わすMatchResultを構築する。
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
	// [itChr,itChrE)のchr_t列を登録されているパターン列で分割し、*pResultに書き込む。
	//	分割は、先頭から貪欲に最長パターン一致で行う。
	void	matchWhole(std::vector<MatchResult> *pResults, std::vector<chr_t>::const_iterator itChr, std::vector<chr_t>::const_iterator itChrE)	const
	{
		std::vector<MatchResult>&	results = *pResults;
		results.clear();

		int	index = INDEX_ROOT;
		for (; itChr != itChrE; itChr++) {
			index = travaseNodeTri(&results, index, *itChr);
		}

		if (index != INDEX_ROOT) {
			//indexがルートでない
			//←マッチ途中
			//→終端させるためにパターンに含まれない文字EOSを読み込ませてindexをルートにまで戻す
			index = travaseNodeTri(&results, index, EOS);

			//「EOSを読み飛ばす」が末尾に出力されているので除去
			results.pop_back();
		}
		assert(index == INDEX_ROOT);
	}

private:

	//	startIndexノードの状態でchr文字を読み込んで状態遷移した先のノードindexを返す。遷移中に確定したマッチ結果を*pResultに追記する。
	int		travaseNodeTri(std::vector<MatchResult> *pResults, int startIndex, chr_t chr)	const
	{
		std::vector<MatchResult>&	results = *pResults;

		int	index = startIndex;
		while (true) {
			std::pair<bool, int>	next = ns.searchNext(index, chr);
			if ( next.first ) {
				//注目文字で遷移できる
				//	遷移して次の文字に進む
				return	next.second;
			}
			else {
				//注目文字で遷移できない
				if (index == INDEX_ROOT) {
					//ルートなので1文字読み飛ばしを出力して次の文字に進む
					results.push_back(MatchResult());
					return	INDEX_ROOT;
				}
				else {
					int	lastDepth;
					if (ns.getMatchIndex(index) >= INDEX_ROOT) {
						//経路上にマッチしたパターンがあった

						//パターン出力
						results.push_back(MatchResult(ns.getDepth(ns.getMatchIndex(index)), ns.getMatchPatternno(ns.getMatchIndex(index))) );

						lastDepth = ns.getDepth(index) - ns.getDepth(ns.getMatchIndex(index));

					}
					else {
						//経路上にマッチしたパターンはなかった
						lastDepth = ns.getDepth(index);
					}

					//fail遷移で注目ノードのdepthが浅くなる = マッチしなかった文字を読み飛ばしている
					//読み飛ばした文字分を出力
					int	skipnum = lastDepth - ns.getDepth(ns.getFailIndex(index) );
					for (int i = 0; i < skipnum; i++) {
						results.push_back(MatchResult());
					}

					//fail遷移して同じ文字でもう一度処理
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
