#ifndef TRIE_EXT_NODE_H
#define TRIE_EXT_NODE_H

#include "TrieNode.h"
#include <vector>

class CTrieExtNode : public CTrieNode
{
public:
	CTrieExtNode& operator = (const CTrieExtNode &t);
	static CTrieNode * genNode(CTrieNode ** baseDataNode);
	CTrieNode ** getPointerToNextPointer();
	bool prepare(bool store);
	CTrieExtNode(CTrieNode * parent);
	CTrieExtNode(CTrieExtNode & sibling);
	CTrieNode * getNext();
	bool setNext(CTrieNode * cnode,bool invalidate=true);//invalidation of node is not needed when doing a DB copy
	std::vector<uint8_t> getNextHash();
private:
    CTrieNode * mNext = NULL;
	
};

#endif
