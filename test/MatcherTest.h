#pragma	once
#include "../src/Matcher.h"
#include <cppunit/extensions/HelperMacros.h>

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

