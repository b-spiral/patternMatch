#pragma	once
#include <stdint.h>
#include <cassert>
#include <cppunit/extensions/HelperMacros.h>

class Matcher
{
public:
	typedef	int32_t	chr_t;
	
	struct MatchResult
	{
		int	size, patternNo;

		//size_文字のchrが、登録されたパターンpatternNo_にマッチする、を表わすMatchResultを構築する。
		MatchResult(int size_, int patternNo_)
			:	size(size_),	patternNo(patternNo_)
		{}

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
	struct Pattern
	{
		std::vector<chr_t>	chrs;
		int	patternNo;
		Pattern(const std::vector<chr_t>& chrs_, int patternNo_)
			: chrs(chrs_), patternNo(patternNo_)
		{}
	};
	std::vector<Pattern>	patterns;

public:
	// [it,it_e)をpatternNoのパターンとして登録する。
	void	addPattern(std::vector<chr_t>::const_iterator it, std::vector<chr_t>::const_iterator it_e, int patternNo)
	{
		patterns.push_back(Pattern(std::vector<chr_t>(it, it_e), patternNo));
	}

	// [itChr,itChrE)のchr_t列を登録されているパターン列で分割し、*pResultに書き込む。
	//	分割は、先頭から貪欲に最長パターン一致で行う。
	void	matchWhole(std::vector<MatchResult> *pResults, std::vector<chr_t>::const_iterator itChr, std::vector<chr_t>::const_iterator itChrE)	const
	{
		std::vector<MatchResult>&	results = *pResults;
		results.clear();

		std::vector<chr_t>::const_iterator itCurChr = itChr;
		while (itCurChr != itChrE) {
			const Pattern*	pMatchPattern = nullptr;
			for (std::vector<Pattern>::const_iterator itPat = patterns.begin(), itPatE = patterns.end(); itPat != itPatE; itPat++) {
				std::vector<chr_t>::const_iterator it = itCurChr, itP = itPat->chrs.begin(), itPE = itPat->chrs.end();
				while (itP != itPE && it != itChrE && *it == *itP) {
					it++;	itP++;
				}
				if (itP == itPE) {
					if (!pMatchPattern || itPat->chrs.size() > pMatchPattern->chrs.size()) {
						pMatchPattern = &(*itPat);
					}
				}
			}
			if (pMatchPattern) {
				size_t size = pMatchPattern->chrs.size();
				results.push_back(MatchResult(size, pMatchPattern->patternNo));
				std::advance(itCurChr, size);
			}
			else {
				results.push_back(MatchResult());
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
	CPPUNIT_TEST_SUITE_END();

private:
	std::auto_ptr<Matcher>	pMatcher;
public:
	void setUp()
	{
		pMatcher = std::auto_ptr<Matcher>(new Matcher());
	}
	void tearDown()
	{
		pMatcher.reset();
	}

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
private:
	void	commonTestParts_matchWhole( const std::string* pat_a, size_t patnum, const std::string& str, const std::string* expect_a, size_t expectnum, const ::CppUnit::SourceLine& sourceLine )
	{
		std::map<std::string, int>	patternToNo;
		for (int i = 0; i<patnum; i++) {
			std::vector<Matcher::chr_t>	chrs(pat_a[i].begin(), pat_a[i].end());
			int	patternNo = i;
			pMatcher->addPattern(chrs.begin(), chrs.end(), i);

			patternToNo.insert(std::make_pair(pat_a[i], i));
		}

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

