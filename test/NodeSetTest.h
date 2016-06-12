#pragma	once
#include "../src/NodeSet.h"
#include <cppunit/extensions/HelperMacros.h>
#include <random>
#include <algorithm>

class NodeSetTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(NodeSetTest);
	CPPUNIT_TEST(test1);
	CPPUNIT_TEST_SUITE_END();

public:
	void test1()
	{
		//óêêîÇ≈ÉmÅ[ÉhóÒÇê∂ê¨
		std::vector<NodeSet::Node>	nodes( 10, NodeSet::Node(0) );
		std::uniform_int_distribution<NodeSet::chr_t>	randChr(1, 0x10);
		std::uniform_int_distribution<int>	randIndex(0, nodes.size() - 1);
		std::uniform_int_distribution<int>	randDepth(1, std::max<int>(2, nodes.size() / 2));
		std::uniform_int_distribution<int>	randMapSize(0, nodes.size() * 2);
		std::uniform_int_distribution<int>	randPatternNo(-1, std::max<int>(1, nodes.size() / 2));
		{
			std::mt19937	mt(123);

			for (int index = 0; index < nodes.size(); index++) {
				NodeSet::Node& node = nodes[index];

				int	mapSize = randMapSize(mt);
				for (int i = 0; i < mapSize; i++) {
					node.chrToNextIndex.insert(std::make_pair(randChr(mt), randIndex(mt)));
				}

				node.depth = (index==0 ? 0 : randDepth(mt));
				node.failIndex = randIndex(mt);
				node.matchIndex = randIndex(mt)-1;
				node.matchPatternno = randPatternNo(mt);
			}
		}

		std::map<int, int>	nodeIdxToNsIdx;
		std::auto_ptr<NodeSet>	pNs(NodeSet::buildFromNodeList(&nodeIdxToNsIdx,nodes));

		CPPUNIT_ASSERT_EQUAL( 0, nodeIdxToNsIdx[0] );
		
		for (int nodeIdx = 0; nodeIdx < nodes.size(); nodeIdx++) {
			NodeSet::Node& node = nodes[nodeIdx];

			int	nsIdx = nodeIdxToNsIdx[nodeIdx];
			CPPUNIT_ASSERT_EQUAL(node.depth, pNs->getDepth(nsIdx));
			CPPUNIT_ASSERT_EQUAL(node.failIndex, pNs->getFailIndex(nsIdx));
			CPPUNIT_ASSERT_EQUAL(node.matchIndex, pNs->getMatchIndex(nsIdx));
			CPPUNIT_ASSERT_EQUAL(node.matchPatternno, pNs->getMatchPatternno(nsIdx));

			const std::map<NodeSet::chr_t, int>& chrToIdx = node.chrToNextIndex;
			if (chrToIdx.size() > 0) {
				std::map<NodeSet::chr_t, int>::const_iterator it, itE = chrToIdx.end();
				std::pair<bool, int>	next;
				for ( it = chrToIdx.begin(); it != itE; it++) {
					next = pNs->searchNext(nsIdx, it->first);
					CPPUNIT_ASSERT_EQUAL(true, next.first);
					CPPUNIT_ASSERT_EQUAL(nodeIdxToNsIdx[it->second], next.second);
				}

				NodeSet::chr_t last_chr = chrToIdx.begin()->first;
				next = pNs->searchNext(nsIdx, last_chr-1);
				CPPUNIT_ASSERT_EQUAL(false, next.first);

				for (it = (chrToIdx.begin())++; it != itE; it++) {
					if (last_chr + 1 < it->first) {
						next = pNs->searchNext(nsIdx, last_chr + 1);
						CPPUNIT_ASSERT_EQUAL(false, next.first);
						next = pNs->searchNext(nsIdx, it->first - 1);
						CPPUNIT_ASSERT_EQUAL(false, next.first);
					}
					last_chr = it->first;
				}

				NodeSet::chr_t chr = chrToIdx.rbegin()->first;
				next = pNs->searchNext(nsIdx, last_chr + 1);
				CPPUNIT_ASSERT_EQUAL(false, next.first);
			}
		}
	}
};

