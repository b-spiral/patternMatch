#pragma	once
#include <stdint.h>
#include <cassert>
#include <cppunit/extensions/HelperMacros.h>

class Matcher
{
public:
	typedef	int32_t	chr_t;

private:
	struct Node
	{
		std::map<chr_t, int>	chrToNextIndex;
		int	matchPatternno;

		Node()
			: matchPatternno(-1)
		{}
	};

public:
	class PatternDictionary
	{
	private:
		std::vector<Node>	nodes;

	public:
		PatternDictionary()
		{
			//index==0������Ă���
			nodes.push_back(Node());
		}
	public:
		// [it,it_e)��patternNo�̃p�^�[���Ƃ��ēo�^����B
		void	addPattern(std::vector<chr_t>::const_iterator itChr, std::vector<chr_t>::const_iterator itChrE, int patternNo)
		{
			assert( patternNo>=0 );
			assert(itChr != itChrE);

			int	index = 0;
			for (; itChr != itChrE; itChr++) {
				Node&	pat = nodes[index];
				std::map<chr_t, int>::iterator	it = pat.chrToNextIndex.find(*itChr);
				if (it == pat.chrToNextIndex.end()) {
					int	next = nodes.size();
					pat.chrToNextIndex.insert(std::make_pair(*itChr,next));

					nodes.push_back(Node());	//pat�������ɂȂ�ꍇ����
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

		std::vector<chr_t>::const_iterator itCurChr = itChr;
		while (itCurChr != itChrE) {
			int	patternno_best = -1;
			std::vector<chr_t>::const_iterator itChr_best;

			//itCurChr�𖖔��܂ŌJ��Ԃ��Ȃ���A���ǂ��Ƃ���܂Ńg���C�����ǂ��Ă���
			int	index = 0;
			const Node	*pNode = &(nodes[index]);
			std::vector<chr_t>::const_iterator itC = itCurChr;
			while ( itC != itChrE ) {
				std::map<chr_t, int>::const_iterator	it = pNode->chrToNextIndex.find(*itC);
				if (it == pNode->chrToNextIndex.end()) {
					//�J�ڐ悪�Ȃ��̂Ŕ�����
					break;
				}

				index = it->second;
				itC++;

				pNode = &(nodes[index]);
				if (pNode->matchPatternno>=0 ) {
					//�J�ڐ悪�p�^�[���Ƀ}�b�`���Ă���B
					//���̐�ł�蒷���p�^�[���Ƀ}�b�`����\��������̂ŁA�ꎞ�L�^���Ă����B
					patternno_best = pNode->matchPatternno;
					itChr_best = itC;
				}
			}

			if (patternno_best >= 0) {
				//�r���Ńp�^�[���Ƀ}�b�`���Ă������Ƃ�����B
				//	�Ŋ��Ƀ}�b�`���Ă����p�^�[�����o��
				results.push_back(MatchResult(std::distance(itCurChr, itChr_best), patternno_best));

				//	�Ŋ��Ƀ}�b�`���Ă�������ɐi��
				itCurChr = itChr_best;
			}else{
				//��x���}�b�`�������Ƃ��Ȃ��B
				//	�}�b�`�Ȃ����o��
				results.push_back(MatchResult());

				//	���ɐi��
				itCurChr++;
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

