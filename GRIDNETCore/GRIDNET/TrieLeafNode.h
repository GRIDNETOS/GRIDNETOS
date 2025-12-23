#ifndef TRIE_LEAF_NODE_H
#define TRIE_LEAF_NODE_H

#include <vector>
#include "TrieExtNode.h"

class CTrieLeafNode :public CTrieNode
{
private:
	//std::string mName;
	
public:

	virtual ~CTrieLeafNode() = default;
	static CTrieLeafNode * genNode(CTrieNode ** baseDataNode);
	CTrieLeafNode(CTrieNode * parent);
	bool setKeyEnd(std::vector<nibblePair> keyEnd);
	bool createTable(uint32_t index);
	bool prepare(bool strore);
	bool setVar( std::vector<uint8_t> key, uint8_t &type, std::vector<uint8_t>  value);
	bool getVarM(std::vector<uint8_t> key, uint8_t &type, std::vector<uint8_t> & value);
	CTrieLeafNode(const CTrieLeafNode& sibling);
	CTrieLeafNode & operator=(const CTrieLeafNode & t);


protected:
};

#endif
