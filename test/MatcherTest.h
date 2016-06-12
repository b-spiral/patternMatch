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
	void	addPattern( const chr_t* it, const chr_t* it_e, int patternNo)
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
		addPattern('a', 'c', 0);
		int	patternNoOf_bab = 1;
		addPattern('b', 'a', 'b', patternNoOf_bab);
		addPattern('a', 'b', 'c', 2);
		addPattern('a', 'b', 'c', 'd', 'e', 3);
		addPattern('d', 'f', 4);

		Matcher::chr_t	chr_a[] = {'x','b','a','b','c','d','e','x'};
		std::vector<Matcher::chr_t>	chrs( chr_a, chr_a+sizeof(chr_a)/sizeof(*chr_a) );

		std::vector<Matcher::MatchResult>	expects;
		expects.push_back(Matcher::MatchResult());		//	x
		expects.push_back(Matcher::MatchResult(3, patternNoOf_bab));	//	b a b
		expects.push_back(Matcher::MatchResult());		//	c
		expects.push_back(Matcher::MatchResult());		//	d
		expects.push_back(Matcher::MatchResult());		//	e
		expects.push_back(Matcher::MatchResult());		//	x

		std::vector<Matcher::MatchResult>	results;
		pMatcher->matchWhole(&results, chrs.begin(), chrs.end());

		CPPUNIT_ASSERT_EQUAL(expects, results);
	}
	void	addPattern(Matcher::chr_t c0, Matcher::chr_t c1, int patternNo)
	{
		const	Matcher::chr_t	c_a[] = { c0, c1 };
		pMatcher->addPattern(c_a, c_a + sizeof(c_a) / sizeof(*c_a), patternNo);
	}
	void	addPattern(Matcher::chr_t c0, Matcher::chr_t c1, Matcher::chr_t c2, int patternNo)
	{
		const	Matcher::chr_t	c_a[] = { c0, c1, c2 };
		pMatcher->addPattern(c_a, c_a + sizeof(c_a) / sizeof(*c_a), patternNo);
	}
	void	addPattern(Matcher::chr_t c0, Matcher::chr_t c1, Matcher::chr_t c2, Matcher::chr_t c3, int patternNo)
	{
		const	Matcher::chr_t	c_a[] = { c0, c1, c2, c3 };
		pMatcher->addPattern(c_a, c_a + sizeof(c_a) / sizeof(*c_a), patternNo);
	}
	void	addPattern(Matcher::chr_t c0, Matcher::chr_t c1, Matcher::chr_t c2, Matcher::chr_t c3, Matcher::chr_t c4, int patternNo)
	{
		const	Matcher::chr_t	c_a[] = { c0, c1, c2, c3, c4 };
		pMatcher->addPattern(c_a, c_a + sizeof(c_a) / sizeof(*c_a), patternNo);
	}
};

