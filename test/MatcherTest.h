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
			//index==0������Ă���
			nodes.push_back(Node(0));
		}
	public:
		// [it,it_e)��patternNo�̃p�^�[���Ƃ��ēo�^����B
		void	addPattern(std::vector<chr_t>::const_iterator itChr, std::vector<chr_t>::const_iterator itChrE, int patternNo)
		{
			assert( patternNo>=0 );
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
					pat.chrToNextIndex.insert(std::make_pair(*itChr,next));

					nodes.push_back(Node(pat.depth+1));	//pat�������ɂȂ�ꍇ����
					index = next;
				}else {
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

		//size_������chr���A�o�^���ꂽ�p�^�[��patternNo_�Ƀ}�b�`����A��\�킷MatchResult���\�z����B
		MatchResult(int size_, int patternNo_)
			:	size(size_),	patternNo(patternNo_)
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
	std::vector<Node>	nodes;

private:
	//	see. PatternDictionary::buildMatcher()
	explicit	Matcher(const std::vector<Node>& nodes_)
		: nodes(nodes_)
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
	int		travaseNodeTri(std::vector<MatchResult> *pResults, int startIndex, chr_t chr )	const
	{
		std::vector<MatchResult>&	results = *pResults;

		int	index = startIndex;
		while (true) {
			const Node&	node = nodes[index];
			std::map<chr_t, int>::const_iterator it = node.chrToNextIndex.find(chr);
			if (it != node.chrToNextIndex.end()) {
				//���ڕ����őJ�ڂł���
				//	�J�ڂ��Ď��̕����ɐi��
				return	it->second;
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
					if (node.matchIndex >= INDEX_ROOT) {
						//�o�H��Ƀ}�b�`�����p�^�[����������
						const	Node&	matchNode = nodes[node.matchIndex];

						//�p�^�[���o��
						results.push_back(MatchResult(matchNode.depth, matchNode.matchPatternno));

						lastDepth = node.depth - matchNode.depth;

					}
					else {
						//�o�H��Ƀ}�b�`�����p�^�[���͂Ȃ�����
						lastDepth = node.depth;
					}

					//fail�J�ڂŒ��ڃm�[�h��depth���󂭂Ȃ� = �}�b�`���Ȃ�����������ǂݔ�΂��Ă���
					//�ǂݔ�΂������������o��
					int	skipnum = lastDepth - nodes[node.failIndex].depth;
					for (int i = 0; i < skipnum; i++) {
						results.push_back(MatchResult());
					}

					//fail�J�ڂ��ē��������ł�����x����
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
//CPPUNIT_ASSERT_EQUAL(expects, results); �𓮍삳���邽�߂̃e���v���[�g���ꉻ
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
			//	�×~�}�b�`�Ȃ̂Ńp�^�[�����̒���abcde����bab���}�b�`����B 
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
			//	�Œ���v�Ȃ̂ŁAabc����abcde���}�b�`����B 
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
			//	abc �̂��� d �܂Ői�݁Ae������� abcde ���o�͂��邪 f�������̂� abc���o�͂��� d�n�_�܂Ŗ߂�Adf�Ƀ}�b�`����B
			//	�߂�n�_���K���łȂ��ƃ}�b�`���Ȃ��Ȃ�B
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
			//	abc �̂��� d �܂Ői�݁Ae������� abcde ���o�͂��邪 �I�[�ɗ����̂� abc���o�͂��� d�n�_�܂Ŗ߂�Adf�Ƀ}�b�`����B
			//	�I�[���l���ł��Ă��Ȃ��A�܂��͖߂�n�_���K���łȂ��ƃ}�b�`���Ȃ��Ȃ�B
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
					//�e�X�g�R�[�h�̃~�X
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

