#ifndef TRIE_BRANCH_NODE_H
#define TRIE_BRANCH_NODE_H

#include <vector>
#include <array>
#include <memory>
#include "TrieNode.h"
#include "TrieExtNode.h"

class CTrieBranchNode :public CTrieNode
{
private:
    std::array< CTrieNode *, 16> mMembers;

    // Lazy-allocated hash data - only allocated when hashes are actually stored
    // Saves ~896 bytes per branch node when not used (common for branches with few children)
    struct HashData {
        std::array<std::vector<uint8_t>, 16> hashes;
    };
    std::unique_ptr<HashData> mHashData;  // NULL until first hash stored

    // Lazy-allocated descriptor data - only allocated when descriptors are used
    // Saves ~512 bytes per branch node when not used (common case)
    struct DescriptorData {
        std::array<std::string, 16> descriptors;
    };
    std::unique_ptr<DescriptorData> mDescriptorData;  // NULL until first descriptor stored

    bool mHashesAvailable = false;

public:
	virtual ~CTrieBranchNode() = default;
	std::string mMemberDescriptor(uint8_t id);
	bool setMemberDescriptor(uint8_t id,std::string description);
	bool verifyHashes();
	bool areHashesAvailable();
	bool recalculateHashes();
	static CTrieBranchNode * genNode(CTrieNode ** baseDataNode);
	std::vector<uint8_t> getMemberHash(uint8_t nibble);
	void clear();
	uint8_t getIndexOfPointer(CTrieNode * pointer);
	std::array< CTrieNode *, 16>  getMembers();
	bool prepare(bool save,bool recalculate=true);
	std::vector<std::vector<uint8_t>> getAllMemberHashes();
	void setAllMemberHashes(std::vector<std::vector<uint8_t>> hashes);
	bool setMember(uint8_t nibble, CTrieNode * node,bool invalidate=true);
	CTrieNode  * getMember(uint8_t nibble);
	CTrieBranchNode(CTrieNode * parent);
	void setMemberHash(uint8_t nibble,std::vector<uint8_t> hash= std::vector<uint8_t>());
	CTrieBranchNode(CTrieBranchNode& sibling);
	CTrieBranchNode& operator = (const CTrieBranchNode &t);
	bool isEmpty();
	bool isEmptyHS();
protected:
};

#endif
