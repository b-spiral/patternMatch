#pragma	once
#include <stdint.h>
#include <cassert>
#include <cppunit/extensions/HelperMacros.h>

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

		explicit	Node( int depth_ )
			:	depth(depth_),	matchPatternno(-1)
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
			assert( patternNo>=0 );
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
					pat.chrToNextIndex.insert(std::make_pair(*itChr,next));

					nodes.push_back(Node(pat.depth+1));	//patが無効になる場合あり
					index = next;
				}else {
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
					while (f >= 0 ) {
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
					queue.push_back( QueueItem(it->second, item.index, it->first) );
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
			:	size(size_),	patternNo(patternNo_)
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
	std::vector<Node>	nodes;

private:
	//	see. PatternDictionary::buildMatcher()
	explicit	Matcher(const std::vector<Node>& nodes_)
		: nodes(nodes_)
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
	int		travaseNodeTri(std::vector<MatchResult> *pResults, int startIndex, chr_t chr )	const
	{
		std::vector<MatchResult>&	results = *pResults;

		int	index = startIndex;
		while (true) {
			const Node&	node = nodes[index];
			std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.find(chr);
			if (it != node.chrToNextIndex.end()) {
				//注目文字で遷移できる
				//	遷移して次の文字に進む
				return	it->second;
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
					if (node.matchIndex >= INDEX_ROOT) {
						//経路上にマッチしたパターンがあった
						const	Node&	matchNode = nodes[node.matchIndex];

						//パターン出力
						results.push_back(MatchResult(matchNode.depth, matchNode.matchPatternno));

						lastDepth = node.depth - matchNode.depth;

					}
					else {
						//経路上にマッチしたパターンはなかった
						lastDepth = node.depth;
					}

					//fail遷移で注目ノードのdepthが浅くなる = マッチしなかった文字を読み飛ばしている
					//読み飛ばした文字分を出力
					int	skipnum = lastDepth - nodes[node.failIndex].depth;
					for (int i = 0; i < skipnum; i++) {
						results.push_back(MatchResult());
					}

					//fail遷移して同じ文字でもう一度処理
					index = node.failIndex;
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
	}else{
		ostm << ", p:unmatch";
	}
	ostm << "}";
	return	ostm;
}


//std::vector<Matcher::MatchResult>	expects, results;
//...
//CPPUNIT_ASSERT_EQUAL(expects, results); を動作させるためのテンプレート特殊化
template<> struct ::CppUnit::assertion_traits< std::vector<Matcher::MatchResult> >
{
	static bool equal(const std::vector<Matcher::MatchResult>& x, const std::vector<Matcher::MatchResult>& y)
	{
		return x == y;
	}

	static std::string toString(const std::vector<Matcher::MatchResult>& x)
	{
		std::ostringstream	ost;
		const char*	sep = "MatchResult[";
		if (x.size()) {
			for (std::vector<Matcher::MatchResult>::const_iterator it = x.begin(), it_f = x.end(); it != it_f; it++) {
				ost << sep << *it;
				sep = ",";
			}
		}else{
			ost << sep;
		}
		ost << "]";
		return ost.str();
	}
};

class MatcherTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(MatcherTest);
	CPPUNIT_TEST(test1);
	CPPUNIT_TEST(test2);
	CPPUNIT_TEST(test3);
	CPPUNIT_TEST(test4);
	CPPUNIT_TEST_SUITE_END();

public:
	void test1()
	{
		std::string	pat_a[] = {
			"ac",	"abc",	"abcde",	"bab",	"df",
		};
		size_t	patnum = sizeof(pat_a) / sizeof(*pat_a);
		std::string	str(
			"xbabcdex"
		);
		std::string	expect_a[] = {
			"x","bab","c","d","e","x"
			//	貪欲マッチなのでパターン長の長いabcdeよりもbabがマッチする。 
		};
		size_t	expectnum = sizeof(expect_a) / sizeof(*expect_a);

		commonTestParts_matchWhole(pat_a, patnum, str, expect_a, expectnum, CPPUNIT_SOURCELINE() );
	}

	void test2()
	{
		std::string	pat_a[] = {
			"ac",	"abc",	"abcde",	"bab",	"df",
		};
		size_t	patnum = sizeof(pat_a) / sizeof(*pat_a);
		std::string	str(
			"xabcdex"
		);
		std::string	expect_a[] = {
			"x","abcde","x"
			//	最長一致なので、abcよりもabcdeがマッチする。 
		};
		size_t	expectnum = sizeof(expect_a) / sizeof(*expect_a);

		commonTestParts_matchWhole(pat_a, patnum, str, expect_a, expectnum, CPPUNIT_SOURCELINE());
	}

	void test3()
	{
		std::string	pat_a[] = {
			"ac",	"abc",	"abcde",	"bab",	"df",
		};
		size_t	patnum = sizeof(pat_a) / sizeof(*pat_a);
		std::string	str(
			"xabcdfx"
		);
		std::string	expect_a[] = {
			"x","abc","df","x"
			//	abc のあと d まで進み、eがくれば abcde を出力するが fが来たので abcを出力して d地点まで戻り、dfにマッチする。
			//	戻る地点が適正でないとマッチしなくなる。
		};
		size_t	expectnum = sizeof(expect_a) / sizeof(*expect_a);

		commonTestParts_matchWhole(pat_a, patnum, str, expect_a, expectnum, CPPUNIT_SOURCELINE());
	}
	
	void test4()
	{
		std::string	pat_a[] = {
			"ac",	"abc",	"abcde",	"bab",	"df",
		};
		size_t	patnum = sizeof(pat_a) / sizeof(*pat_a);
		std::string	str(
			"xabcd"
		);
		std::string	expect_a[] = {
			"x","abc","d"
			//	abc のあと d まで進み、eがくれば abcde を出力するが 終端に来たので abcを出力して d地点まで戻り、dfにマッチする。
			//	終端が考慮できていない、または戻る地点が適正でないとマッチしなくなる。
		};
		size_t	expectnum = sizeof(expect_a) / sizeof(*expect_a);

		commonTestParts_matchWhole(pat_a, patnum, str, expect_a, expectnum, CPPUNIT_SOURCELINE());
	}

private:
	void	commonTestParts_matchWhole( const std::string* pat_a, size_t patnum, const std::string& str, const std::string* expect_a, size_t expectnum, const ::CppUnit::SourceLine& sourceLine )
	{
		Matcher::PatternDictionary	dictionary;
		
		std::map<std::string, int>	patternToNo;
		for (int i = 0; i<patnum; i++) {
			std::vector<Matcher::chr_t>	chrs(pat_a[i].begin(), pat_a[i].end());
			int	patternNo = i;
			dictionary.addPattern(chrs.begin(), chrs.end(), i);

			patternToNo.insert(std::make_pair(pat_a[i], i));
		}
		std::auto_ptr<Matcher>	pMatcher( dictionary.buildMatcher() );

		std::vector<Matcher::MatchResult>	expects;
		for (int i = 0; i < expectnum; i++) {
			const std::string&	expect = expect_a[i];
			std::map<std::string, int>::const_iterator it = patternToNo.find(expect);
			if (it != patternToNo.end()) {
				expects.push_back(Matcher::MatchResult(expect.size(), it->second));
			}else{
				if (expect.size() == 1) {
					expects.push_back(Matcher::MatchResult());
				}else{
					//テストコードのミス
					CPPUNIT_FAIL("expect ["+expect+"] is not registed.");
				}
			}
		}

		std::vector<Matcher::chr_t>	chrs(str.begin(), str.end());

		std::vector<Matcher::MatchResult>	results;
		pMatcher->matchWhole(&results, chrs.begin(), chrs.end());

		CPPUNIT_NS::assertEquals(expects, results, sourceLine, "");
	}
};

