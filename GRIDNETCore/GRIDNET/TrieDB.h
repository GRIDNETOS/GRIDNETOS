#ifndef TRIE_DB_H
#define TRIE_DB_H

#include <stdlib.h>
#include <algorithm>   
#include <vector>
#include "TrieNode.h"
#include "TrieLeafNode.h"
#include "reverseSemaphore.h"
#include <random>
#include <limits>  

#include "TrieBranchNode.h"
#include "TrieExtNode.h"
#include <regex>
#include "robin_hood.h"
class CSolidStorage;
class ThreadPool;
/*		The Truth of Faith
1) When changing perspective of a TrieDB, all pointers to any its nodes would become INVALID (deleted).
Thus, one needs to refresh the pointer (find node by its key once again).
2) When modiyfing variables of a State Domain, the perspective of any instance of a StateTrie which is not the
inFlowStateTrie (pointed to by a pointer within the State Domain) needs to be refreshed.
3) inFlowStateTrie allows to maintain state accross State Domains while within the Flow.
4) Thus, Transaction Manager, StateDomainManager within the former contain a single instance of an inFlowStateTrie
(marked as such by a boolean flag).
5) If, a TrieDB marked as inFlowStateDB instantiates a StateDomain it would set itself as an inFlowStateTrie within it.
*/


/*
# Merkle Patricia Trie Documentation

## Table of Contents
1. [Introduction](#introduction)
2. [Node Types](#node-types)
3. [Nibbles and Partial Paths](#nibbles-and-partial-paths)
4. [Node Invalidation](#node-invalidation)
5. [Hash Calculation](#hash-calculation)
6. [Node Serialization and Deserialization](#node-serialization-and-deserialization)
7. [State Domains](#state-domains)
8. [Security and Ownership](#security-and-ownership)
9. [Important Considerations](#important-considerations)

## 1. Introduction

The Merkle Patricia Trie is a fundamental data structure used in blockchain systems, particularly in Ethereum-like environments. It combines features of a Radix trie and a Merkle tree to create an efficient and cryptographically verifiable key-value store.

## 2. Node Types

The Merkle Patricia Trie consists of three main types of nodes:

### 2.1 Branch Nodes (Type 2)
- Represent a branching point in the trie.
- Can have up to 16 children (one for each possible nibble value).
- Used to navigate through the trie based on the key's nibbles.

### 2.2 Extension Nodes (Type 1)
- Optimize the trie by compressing long paths with only single child.
- Contain a partial key (shared nibbles) and a single child.

### 2.3 Leaf Nodes (Type 3)
- Store the actual key-value pairs.
- Can have different subtypes (e.g., RAW, StateDomain, Transaction, Receipt, Verifiable, TrieDB).

## 3. Nibbles and Partial Paths

### 3.1 Nibbles
- A nibble is a 4-bit unit (half a byte).
- Used to represent keys in a more granular way than full bytes.
- Allow for efficient partial matching and compact representation of keys.

### 3.2 Partial Paths
- Represented by the `mInterData` field in nodes.
- For leaf and extension nodes, contain the remaining part of the key.
- For branch nodes, typically empty.

### 3.3 Full ID
- Constructed by combining partial paths from the root to a specific node.
- Used for node identification and integrity verification.

## 4. Node Invalidation

Nodes are invalidated when their content or structure changes:

- `invalidateNode()`: Marks the current node as not prepared and clears its precomputed hash.
- `invalidatePath()`: Invalidates the entire path from the current node to the root.

Invalidation triggers include:
- Changing node type or subtype
- Modifying raw data or partial ID
- Updating security descriptors

## 5. Hash Calculation

- Hashes are crucial for maintaining trie integrity and enabling Merkle proofs.
- The `getHash()` method calculates and caches the node's hash.
- Special consideration is given to StateDomain nodes, where certain fields don't affect the hash.

## 6. Node Serialization and Deserialization

### 6.1 Serialization (`getPackedData()`)
- Converts node data into a byte vector for storage or transmission.
- Includes node version, type, partial ID, and raw data.
- For leaf nodes, also includes name, security descriptor, and metadata.

### 6.2 Deserialization (`unpackData()`, `nodeFromBytes()`)
- Reconstructs nodes from serialized data.
- Handles different node types and subtypes.

## 7. State Domains

State Domains are special leaf nodes (subtype 1) with unique properties:

- Can be moved within the trie without changing their hash.
- Require special handling for partial ID updates.
- Contain their own internal databases.

## 8. Security and Ownership

- Nodes can have associated security descriptors (`CSecDescriptor`).
- Ownership is determined by the nearest owner up the trie hierarchy.
- Security policies can be inherited from parent nodes.

## 9. Important Considerations

1. **Partial ID Updates**: When nodes are moved or the trie structure changes, partial IDs must be updated to maintain consistency.

2. **Hash Stability**: Some node types (e.g., StateDomains) maintain stable hashes even when their position in the trie changes.

3. **Prepared State**: Nodes have a "prepared" state (`mIsPrepared`) indicating whether their packed data and hash are up-to-date.

4. **Thread Safety**: The implementation uses mutex locks to ensure thread-safe operations on nodes.

5. **Cold Storage Integration**: The trie supports interaction with a cold storage system, allowing nodes to be loaded on-demand.

6. **Integrity Verification**: The trie structure allows for efficient verification of data integrity using Merkle proofs.

7. **Extensibility**: The node structure supports various subtypes, allowing for specialized behavior (e.g., transactions, receipts).

8. **Memory Management**: Developers should be aware of potential memory leaks, especially when dealing with pointer-based node relationships.

9. **Performance Considerations**: Operations like node invalidation can propagate up the trie, potentially affecting performance for large tries.

10. **Serialization Format**: The trie uses a custom serialization format based on BER encoding, which developers need to understand for interoperability.
*/


/*
# State Domain Handling in Merkle Patricia Trie: Rationale and Implementation

## 1. Introduction

State Domains are special leaf nodes in the Merkle Patricia Trie that represent distinct areas of state within the blockchain system. Their unique properties and requirements necessitate careful handling to maintain both flexibility and integrity within the trie structure.

## 2. Key Characteristics of State Domains

1. **Mobility**: State Domains can be moved around within the trie.
2. **Address-based Positioning**: Their position is determined by their address, not their hash.
3. **Hash Stability**: The hash of a State Domain should remain constant even if its position changes.
4. **Integrity Verification**: The trie structure must still allow for verification of data integrity.

## 3. The Challenge: Balancing Mobility and Integrity

The core challenge lies in allowing State Domains to be mobile within the trie while maintaining a consistent hash and ensuring the trie's integrity can still be verified.

### 3.1 The mInterData Field

- The `mInterData` field in trie nodes typically contains the partial path (suffix) of the node's position in the trie.
- For most nodes, this is part of the node's identity and affects its hash.
- For State Domains, we want this to be mutable without affecting the node's hash.

### 3.2 Hash Calculation

- The hash of a State Domain is based on its critical values protected by Global Consensus.
- It should not change when the domain is moved, even though its `mInterData` (partial path) changes.

## 4. The Solution: Prepared State and Partial ID Updates

The implemented solution involves a two-step process:

1. **Marking as Prepared**: When a State Domain is deserialized, it's marked as prepared.
2. **Updating Partial ID**: When instantiated through TrieDB, its partial ID is updated separately.

### 4.1 Rationale for This Approach

1. **Separation of Concerns**:
   - Deserialization deals with the domain's content.
   - Trie insertion deals with the domain's position.

2. **Flexibility**:
   - Allows State Domains to be moved without requiring re-serialization.
   - Maintains a consistent hash regardless of position.

3. **Integrity Preservation**:
   - The trie structure still contains correct paths for integrity verification.
   - The State Domain's hash remains stable for cross-referencing and retrieval.

## 5. Implementation Details

### 5.1 Deserialization Process

```cpp
if (toRet != nullptr)
	toRet->markAsPrepared(true);
```

- After deserialization, the State Domain is marked as prepared.
- This indicates that its core data is ready and its hash is valid.

### 5.2 Trie Insertion Process

```cpp
// Pseudo-code for insertion
stateDomain->updatePartialID(fullPath);
```

- When inserted into the trie, `updatePartialID()` is called.
- This updates the `mInterData` field without invalidating the node or changing its hash.

## 6. Consequences and Considerations

### 6.1 Performance Impact

- Slight performance cost due to additional `updatePartialID()` call during insertion.
- Benefit: Avoids need for re-serialization or hash recalculation on moves.

### 6.2 Storage Efficiency

- Avoids storing multiple serialized versions of the same State Domain with different partial IDs.
- Trades minor computational cost for significant storage savings.

### 6.3 Integrity Verification

- Trie integrity checks must be aware of this special handling for State Domains.
- Verification processes should use the domain's address rather than relying solely on the trie path.

## 7. Alternative Approaches Considered

### 7.1 Storing Multiple Serialized Versions

- **Approach**: Store separate serialized representations for each possible partial ID.
- **Pros**: Simpler retrieval, no need for partial ID updates.
- **Cons**: Significant storage overhead, potential consistency issues.

### 7.2 Excluding Partial ID from Serialization

- **Approach**: Don't include `mInterData` in State Domain serialization at all.
- **Pros**: Simplifies mobility.
- **Cons**: Complicates trie structure and verification processes.

## 8. Conclusion

The implemented approach for handling State Domains in the Merkle Patricia Trie represents a carefully balanced solution. It maintains the integrity and verifiability of the trie structure while allowing for the unique mobility requirements of State Domains. By separating the concerns of data integrity (handled during deserialization) and trie position (handled during insertion), the system achieves flexibility without compromising on the core principles of a Merkle Patricia Trie.

This solution does introduce some complexity in terms of node handling and requires developers to be aware of the special nature of State Domains. However, it provides significant benefits in terms of storage efficiency and operational flexibility, which are crucial in a blockchain environment where state management is a critical concern.
*/

/*
**Understanding Nibbles and Their Role in Trie Traversal**

---

**Introduction:**

In your trie traversal algorithm, particularly in the context of a Merkle Patricia Trie, the concept of **nibbles** (half-bytes) plays a crucial role. Nibbles are used to represent paths within the trie, allowing for efficient and accurate reconstruction of keys (identifiers) associated with leaf nodes. Let's delve into the critical components and mechanics of your implementation, focusing on the use of nibbles and how they contribute to the overall operation of the trie traversal.

---

### **What Are Nibbles and Why Are They Needed?**

- **Definition of a Nibble:**
  - A **nibble** is a 4-bit aggregation, which is half of an 8-bit byte.
  - It can represent a hexadecimal digit (since hex digits range from 0x0 to 0xF).

- **Use in Tries:**
  - In trie data structures, especially in Merkle Patricia Tries used in blockchain systems like Ethereum, keys are represented as sequences of nibbles.
  - Nibbles allow for finer granularity than bytes, enabling efficient storage and retrieval of data based on partial key matches.

- **Why Nibbles Are Needed:**
  - **Efficient Key Representation:**
	- Using nibbles reduces the depth of the trie compared to using bits, balancing between space efficiency and lookup speed.
  - **Partial Matching:**
	- Nibbles facilitate partial matching of keys, which is essential in tries where keys can share common prefixes.
  - **Hexadecimal Keys:**
	- Blockchain addresses and keys are often represented in hexadecimal, making nibbles a natural fit.

---

### **Critical Components and Mechanics of the Trie Traversal**

1. **`nibblePair` Structure:**

   - **Purpose:**
	 - Represents one or two nibbles in a single unit.
   - **Fields:**
	 - `mA`: The value of the first nibble.
	 - `mB`: The value of the second nibble (if present).
	 - `mHasRight`: A boolean indicating if the second nibble (`mB`) is present.
   - **Usage:**
	 - Accumulates the path as a sequence of `nibblePair`s while traversing the trie.
	 - Allows efficient storage and manipulation of paths.

2. **Partial Path Accumulation:**

   - **Concept:**
	 - As you traverse the trie from the root to a leaf node, you build up a **partial path** representing the key associated with that leaf.
   - **Mechanism:**
	 - **Extension Nodes:**
	   - Contain a sequence of nibbles in their `partialID`.
	   - When you encounter an extension node, you append its `partialID` to the current path.
	 - **Branch Nodes:**
	   - Represent a branching point based on a single nibble (the index from 0 to 15).
	   - When moving to a child node, you append the nibble corresponding to the child's index to the path.

3. **Updating the Partial Path Correctly:**

   - **The Issue and Its Resolution:**
	 - **Issue:**
	   - In the multi-threaded version, the path passed to `validateNextNodeAvailability` was missing the nibble representing the child index when processing branch nodes.
	   - This omission led to incomplete paths and invalid identifiers.
	 - **Resolution:**
	   - **Append the Offspring Node's Nibble:**
		 - Before calling `validateNextNodeAvailability`, you need to append the current index (as a nibble) to the `partialPath`.
		 - This ensures that the path accurately reflects the traversal up to the child node.

   - **Code Example:**

	 // Correctly appending the nibble for branch nodes
	 nibblePair n;
	 n.mA = i;          // 'i' is the index of the child (0-15)
	 n.mB = 0;
	 n.mHasRight = false;

	 std::vector<nibblePair> childPath = currentPath;
	 childPath.push_back(n); // Append the nibble to the path

	 // Pass 'childPath' to validateNextNodeAvailability
	 if (validateNextNodeAvailability(branchNode, i, false, childPath)) {
		 // Enqueue the child node with the updated path
		 toBeEnqueued.emplace_back(nextNode, childPath);
	 }


4. **Constructing the Final Identifier:**

   - **At Leaf Nodes:**
	 - When a leaf node is reached, the accumulated `partialPath` represents the full key (identifier) for that node.
	 - The sequence of nibbles is converted into bytes to form the identifier.
   - **Conversion Mechanism:**
	 - **Combining Nibbles into Bytes:**
	   - Since each nibble is 4 bits, two nibbles can be combined to form a single byte.
	   - If there's an odd number of nibbles, the last byte will only use the least significant nibble.
	 - **Handling Least Significant Nibble:**
	   - A flag `hasLeastSignificantNibble` indicates if the last byte is incomplete.

   - **Code Example:**

	 // At a leaf node
	 mTools->addEndingNibbles(currentNode->getPartialID(), currentPath);

	 bool hasLeastSignificantNibble = false;
	 std::vector<uint8_t> id = mTools->nibblesToBytes(currentPath, hasLeastSignificantNibble);

	 // 'id' now contains the final identifier


5. **Thread Safety and Independent Paths:**

   - **Ensuring Correctness in Multi-Threaded Traversal:**
	 - Each thread maintains its own `partialPath`, preventing interference between threads.
	 - Nodes are not modified in a way that affects other threads.
   - **Avoiding Shared State Modifications:**
	 - By passing `partialPath` as a parameter and not modifying shared node data, race conditions are avoided.
	 - Synchronization mechanisms (like mutexes) are used where necessary to protect shared resources.

---

### **The Mechanics of the Multi-Threaded Traversal**

1. **Task Queue and Workers:**

   - **Task Queue:**
	 - Stores pairs of nodes and their associated `partialPath`s.
	 - Workers retrieve tasks from the queue, process nodes, and enqueue new tasks as needed.
   - **Workers:**
	 - Multiple threads execute the traversal concurrently.
	 - Each worker processes nodes independently, using its own copy of the `partialPath`.

2. **Processing Nodes:**

   - **Branch Nodes:**
	 - For each non-null child, append the child's index as a nibble to the `partialPath`.
	 - Enqueue the child node with the updated path.
   - **Extension Nodes:**
	 - Append the node's `partialID` to the `partialPath`.
	 - Enqueue the next node with the updated path.
   - **Leaf Nodes:**
	 - Use the accumulated `partialPath` to construct the identifier.

3. **Avoiding the Original Issue:**

   - **Original Issue:**
	 - Not appending the offspring node's nibble led to incomplete paths.
   - **Resolution in Mechanics:**
	 - Ensure that at every step, the `partialPath` is correctly updated to reflect the traversal.
	 - This involves adding the current node's `partialID` or the child's index as a nibble before processing or enqueuing child nodes.

---

### **Summary and Conclusion**

- **Importance of Nibbles:**
  - Nibbles provide a natural and efficient way to represent hexadecimal keys in tries.
  - They allow for accurate reconstruction of keys during traversal.

- **Critical Components:**
  - **`nibblePair` Structure:** Fundamental for representing paths.
  - **Partial Path Accumulation:** Essential for building identifiers.
  - **Identifier Construction:** Converts the path into usable keys.
  - **Thread Safety:** Ensures correctness in a multi-threaded environment.

- **Mechanics of Operation:**
  - By correctly managing and updating the `partialPath`, the traversal accurately reflects the structure of the trie.
  - Each node's contribution to the path is accounted for, ensuring that identifiers are valid.
  - The fix involved properly appending the offspring node's nibble to the `partialPath`, aligning the multi-threaded version with the single-threaded logic.

---


### **Additional Considerations**

- **Memory Efficiency:**
  - Using nibbles allows for compact representation of paths, which is beneficial in large tries.
- **Performance:**
  - Proper management of `partialPath` ensures that the traversal is efficient and avoids unnecessary overhead.
- **Scalability:**
  - The multi-threaded implementation, when correctly handling paths, scales well with the number of threads.

---

### **Final Thoughts**

Understanding the role of nibbles and the mechanics of path accumulation is crucial in trie traversal algorithms, especially in complex structures like Merkle Patricia Tries. By ensuring that each step of the traversal accurately updates the `partialPath`, you maintain the integrity of the identifiers and the correctness of the traversal, whether in a single-threaded or multi-threaded context.

---

**References:**

- **Trie Data Structures:** Fundamental for understanding prefix trees and their traversal.
- **Merkle Patricia Trie:** Used in blockchain implementations for efficient state storage.
- **Concurrency in Data Structures:** Important for multi-threaded implementations to ensure thread safety and correctness.

---

I hope this explanation clarifies the critical components and mechanics of your trie traversal algorithm, the importance of nibbles, and how they are used to construct valid identifiers in both single-threaded and multi-threaded environments.
*/


class ActiveTaskGuard {
public:
	ActiveTaskGuard(std::atomic<size_t>& activeTasks, std::mutex& queueMutex, std::condition_variable& queueCV)
		: activeTasks_(activeTasks), queueMutex_(queueMutex), queueCV_(queueCV), active_(true) {}

	void deactivate() {
		std::lock_guard<std::mutex> lock(mGuardian);
		if (active_) {
			active_ = false;
			if (activeTasks_.fetch_sub(1, std::memory_order_relaxed) == 1) {
				// Lock the queueMutex before notifying
				std::unique_lock<std::mutex> queueLock(queueMutex_);
				queueCV_.notify_all();
				// Mutex is released when queueLock goes out of scope
			}
		}
	}

	~ActiveTaskGuard() {
		deactivate();
	}

private:
	std::atomic<size_t>& activeTasks_;
	std::mutex& queueMutex_;
	std::condition_variable& queueCV_;
	bool active_;
	std::mutex mGuardian;
};


class CTrieDB : public CTrieLeafNode
{

protected:
// UpdateGuard - BEGIN
	// RAII guard for update status
	class UpdateGuard {
	private:
		CTrieDB& mTrie;
		bool mActive;

	public:
		explicit UpdateGuard(CTrieDB& trie) : mTrie(trie), mActive(false) {
			std::unique_lock<std::shared_mutex> lock(mTrie.mUpdateMutex);
			if (!mTrie.mIsBeingUpdated.load(std::memory_order_acquire)) {
				mTrie.mIsBeingUpdated.store(true, std::memory_order_release);
				mActive = true;
			}
			else {
				throw std::runtime_error("Trie is already being updated");
			}
		}

		~UpdateGuard() {
			if (mActive) {
				std::unique_lock<std::shared_mutex> lock(mTrie.mUpdateMutex);
				mTrie.mIsBeingUpdated.store(false, std::memory_order_release);
				mTrie.mUpdateCV.notify_all();
			}
		}

		UpdateGuard(const UpdateGuard&) = delete;
		UpdateGuard& operator=(const UpdateGuard&) = delete;
		UpdateGuard(UpdateGuard&&) = delete;
		UpdateGuard& operator=(UpdateGuard&&) = delete;
	};
	// UpdateGuard - END
public:
	enum eResult {
		added,
		updated,
		failure
	};

	bool isBeingUpdated() const;
	std::vector<uint8_t> getAssociatedObjectID();
	void setAssociatedObjectID(std::vector<uint8_t>& objectID);
	void clearAssociatedObjectI();
	// Wait until the trie is not being updated
	void waitForUpdateCompletion();
	bool getIsTrieFullyInRAM();
	void setIsTrieFullyInRAM(bool isIt = true);
	static std::shared_ptr<ThreadPool> globalThreadPool;
protected:
	std::string mSsPrefix;
	std::vector<uint8_t> mRootHash;
	//std::string mName;
	size_t mVersion;

private:
	mutable std::mutex mNodeAvailabilityCheckGuardian;
	mutable  std::shared_mutex mSharedGuardian;
	std::vector<uint8_t> mAssociatedObjectID;
	bool mTrieInRAM;
	std::mutex mFieldsGuardian;
	// Updates' Support - BEGIN
	std::atomic<bool> mIsBeingUpdated{ false };
	mutable std::shared_mutex mUpdateMutex;  // Reader-writer lock for update status
	std::condition_variable_any mUpdateCV;  // CV that works with shared_mutex


	// Updates' Support - END

	robin_hood::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>> mInitialDomainBodies;// this map is used to hold initial hashes of State Domains that
	//are to be modified within a Flow. After the flow, these State Domains are retrieved from the StateTrie, the hash of their previous (unmodified) states are added
	//to their backlogs and the domains are written down to the Solid Storage AGAIN. (the backlog does NOT affect the domain's hash i.e. or the key under which
	//it is stored)
	// Update: now, these hashes are 'versioned'. The list of hashes is associated with a particular instance of a CTrieDB. Thus, once we perform an ACID roll-back
	//         the list of modified state-domains is rolled back as well. Otherwise, we would be accessing domains which are no longer part of the current state
	//         ( one after a roll-back).

	bool mIsInFlowStateDB;//is this an In-FLow inter-domain state db? if so it will be set as such in each state domain instantiated within this Trie


	//std::mutex mGuardian;
	CTrieNode* mRoot = NULL;

	std::shared_ptr<CTools> mTools;
	bool verifyTriePath(std::vector<uint8_t> rootHash, CTrieNode* child, std::vector<uint8_t> previousHash);

	CTrieNode* getNodeWithID(CTrieNode* node, std::vector<uint8_t> id);
	bool testTrie(CTrieNode* n, size_t& nodeCount, bool doActivePruning = true, bool flashCurrentCount = false, std::vector<nibblePair> id = std::vector<nibblePair>(), uint64_t maxNrofNodesTotest = 0);

	/**
	 * @brief Performs a multi-threaded traversal and testing of a Merkle Patricia Trie.
	 *
	 * This method is designed to efficiently traverse and test a Merkle Patricia Trie
	 * structure using multiple threads. Its main aim is to find and process elements
	 * in the trie while performing various checks and operations.
	 *
	 * Key features:
	 * - Multi-threaded traversal for improved performance on large tries
	 * - Optional active pruning of the trie during traversal
	 * - Ability to limit the number of nodes processed
	 * - Progress tracking and status updates
	 * - Comprehensive error handling and thread safety
	 *
	 * The method traverses the trie, processing each node type (extension, branch, and leaf)
	 * accordingly. It can perform integrity checks, count nodes, and optionally prune
	 * the trie structure.
	 *
	 * @param node Pointer to the root node of the Merkle Patricia Trie to be tested.
	 * @param nodeCount Reference to a variable that will be updated with the total number
	 *                  of nodes processed during the traversal.
	 * @param doActivePruning If true, performs pruning operations on the trie during traversal.
	 * @param flashCurrentCount If true, periodically updates a status bar with the current
	 *                          node count.
	 * @param id Initial vector of nibble pairs representing the starting path in the trie.
	 * @param maxNrofNodesTotest Maximum number of nodes to process before stopping the traversal.
	 *                           If set to 0, processes all nodes in the trie.
	 * @param numThreads Number of worker threads to use for parallel processing. Defaults to 8.
	 *
	 * @return true if the traversal completes successfully, false if the input node is null.
	 *
	 * @throws Rethrows any exceptions caught during the traversal process.
	 *
	 * @note This method uses atomic operations and mutex locks to ensure thread safety.
	 *       It's designed to handle large trie structures efficiently by distributing
	 *       the workload across multiple threads.
	 *
	 * @warning This method may modify the trie structure if doActivePruning is set to true.
	 *
	 * Usage example:
	 * @code
	 * CTrieNode* rootNode = GetTrieRootNode();
	 * size_t totalNodes = 0;
	 * std::vector<nibblePair> startPath;
	 * bool success = testTrieMT(rootNode, totalNodes, true, true, startPath, 1000000, 16);
	 * if (success) {
	 *     std::cout << "Processed " << totalNodes << " nodes in the trie." << std::endl;
	 * }
	 * @endcode
	 */
	bool testTrieMT(CTrieNode* node, size_t& nodeCount, bool doActivePruning = true, bool flashCurrentCount = false, std::vector<nibblePair> id = std::vector<nibblePair>(), uint64_t maxNrofNodesTotest = 0, size_t numThreads = 8);

	// Node availability Checks - BEGIN

	bool validateNextNodeAvailability(CTrieExtNode* extNode, bool disableIntegrityChecks = false, const std::vector<nibblePair> partialPath = std::vector<nibblePair>());
	bool validateNextNodeAvailabilityK(CTrieExtNode* extNode, bool disableIntegrityChecks = false, const std::vector<nibblePair> partialPath = std::vector<nibblePair>());
	bool validateNextNodeAvailability(CTrieBranchNode* branchNode, uint8_t index, bool disableIntegrityChecks = false, const std::vector<nibblePair> partialPath = std::vector<nibblePair>());
	bool validateNextNodeAvailabilityK(CTrieBranchNode* branchNode, uint8_t index, bool disableIntegrityChecks = false, const std::vector<nibblePair> partialPath = std::vector<nibblePair>());
	// Node availability Checks - END
	struct BranchIndexKey {
		CTrieBranchNode* branch;
		uint8_t index;

		bool operator==(const BranchIndexKey& other) const {
			return branch == other.branch && index == other.index;
		}
	};


	struct BranchIndexKeyHash {
		std::size_t operator()(const BranchIndexKey& key) const {
			return std::hash<void*>()((void*)key.branch) ^ std::hash<uint8_t>()(key.index);
		}
	};
	struct ExtNodeKey {
		CTrieExtNode* extNode;

		bool operator==(const ExtNodeKey& other) const {
			return extNode == other.extNode;
		}
	};

	// Add this hash function for the key (if not already present)
	struct ExtNodeKeyHash {
		std::size_t operator()(const ExtNodeKey& key) const {
			return std::hash<void*>()((void*)key.extNode);
		}
	};
	robin_hood::unordered_map<ExtNodeKey, std::unique_ptr<std::mutex>, ExtNodeKeyHash> mExtNodeDeserializationMutexes;
	std::mutex mExtNodeDeserializationMapMutex;
	robin_hood::unordered_map<BranchIndexKey, std::unique_ptr<std::mutex>, BranchIndexKeyHash> mNodeDeserializationMutexes;

	std::mutex mNodeDeserializationMapMutex;
	CSolidStorage* mSs;
	std::vector<uint8_t> getPackedMerklePathBottomTop(CTrieNode* root, CTrieNode* current, std::vector<std::vector<uint8_t>> interState);
	std::vector<uint8_t> getPackedMerklePathBottomTop(CTrieNode* root, CTrieLeafNode* leaf);
	std::vector<uint8_t> getPackedMerklePathTopBottom(std::vector<nibblePair> path, CTrieNode* root); //root can be specified
	std::vector<uint8_t> getPackedMerklePathTopBottom(std::vector<nibblePair> path, CTrieNode* node, std::vector<CTrieNode*> nodes); //used by recursion
	CTrieNode* findNodeByFullID(std::vector<nibblePair> path, CTrieNode* root, bool disableIntegrityChecks = false, bool disablePerspectiveUpdate = false);
	CTrieNode* findNodeByFullID(std::vector<nibblePair> path, CTrieNode* root, std::vector<CTrieNode*>& nodesTraversed, bool disableIntegrityChecks = false, bool disablePerspectiveUpdate = false);


	void postAdditionTrieSync(eResult result, CTrieNode* node, CTrieNode* root, std::vector<CTrieNode*>& currentlyModifiedNodes, std::vector<CTrieNode*>& currentlyModifiedNodesLocal, bool sandBoxMode = false);
	bool postModificationSync(std::vector<nibblePair> id, std::vector<uint8_t> rootID, bool sandBoxMode = false);
	CTrieDB::eResult addNode(std::vector<nibblePair> idPart, CTrieNode* root,
		CTrieNode* current, CTrieNode* toAdd, std::vector<CTrieNode*>& currentlyModifiedNodes, bool sandBoxMode = false, bool disableIntegrityChecks = false, bool  justSetSecToken = false);


	CTrieDB::eResult addNode(std::vector<nibblePair> idPart, CTrieNode* root,
		CTrieNode* current, CTrieNode* toAdd, bool sandBoxModex = false, bool disableIntegrityChecks = false);
	bool sliceNode(CTrieNode* cnode);
	std::vector<std::vector<uint8_t>> mKnownLeafNodeIDs;
	void getHotStorageLeaves(CTrieNode* node, std::vector<CTrieNode*>& leaves, std::vector<nibblePair> id);
	bool mTrackKnownNodes;
	bool copyTo(CTrieDB* target, CTrieNode* currentNode, CTrieNode* currentNodeInTarget, bool onlyHotStorage = true);
public:
	std::string getName();
	void clearStateDomainsModifiedDuringTheFlow();
	bool setNamePrefix(std::string name);

	//SYNCHRONIZATION  CONSTRUCTS - BEGIN
									//**IMPORTANT**: DataGuardian (IF needed) - it needs to be locked BEFORE mGuardian.
	//std::recursive_mutex mGuardian;// this recursive mutex prevent simultaneous reads (these are fine) and *writes*.  
								   // BUT - DataGuardian may NOT be needed. Mutex mGuardian is locked per operation, while DataGuardian is locked for beyond of hte scope of an operation.
								   // Thus, DataGuardian protects the lifetime of internal objects stored within of the Trie. mGuardian can be thus said to protect the Trie itself, while
								   // DataGuardian protects its elements.

	ReverseSemaphore DataGuardian;// this Reverse Semaphore prevents the TrieDB from being wiped-out WHILE other external objects hold references to sub-objects of this CTrieDB.
	//The need for this stems from the fact that all the look-up functions return pointers to sub-objects instead of their copies.
	//This may be replied upon only in the case of LIVE Transaction manager as it is guaranteed to have immutable sub-objects.
	//In the case of LIVE Transaction Manager, the contents of the Trie changes only on a call to setPerspective().
	//We thus use mReverseSemaphore, to make  setPerspective() wait until the semaphore is free. Notice that multiple threads may lock
	//mReverseSemaphore simultaneously in a thread safe manner.  The waitFree() function would wait until external threads are done.
	//^
	//**IMPORTANT**: DataGuardian needs to be locked BEFORE mGuardian.
//SYNCHRONIZATION  CONSTRUCTS - BEGIN

	void lock();
	void unlock();
	bool try_lock();
	bool copyTo(CTrieDB* target, bool onlyHotStorage = true, bool doIntegrityCheck = false);
	CTrieDB& operator = (const CTrieDB& t);
	void recalcPerspective();
	void setFlowStateDB(bool is = true);
	bool isInFlowStateDB();
	bool isTestDB();
	std::string getSSPrefix();
	CTrieDB(const CTrieDB& sibling);

	bool isEmpty(bool checkColdStorage = true);
	void trackKnownNodes(bool track = true);
	bool postModificationSync(std::vector<nibblePair> fullID, CTrieNode* node = NULL, bool sandBoxMode = false);
	bool removeNode(std::vector<nibblePair> id, bool sandBoxModex = false);
	std::vector<uint8_t> getPerspective();
	bool forceRootNodeStorage();
	CTrieDB(std::string ssPrefix, CSolidStorage* mSS, std::vector<uint8_t>rootHash = std::vector<uint8_t>(), bool trackNodes = false, bool throwOnError = true, bool loadRootsFromSS = true);

	bool setPerspective(std::vector<uint8_t> rootID, const robin_hood::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>>& modifiedDomainsInPreviousFlow = robin_hood::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>>());
	robin_hood::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>>& getModifiedDomainsKer();
	CTrieDB(CSolidStorage* ss_a, std::vector<uint8_t>rootHash, std::string name = "main");
	virtual ~CTrieDB();
	CSolidStorage* getSS();
	bool clearRAMCache();
	void setCurrentTrieRoot(const CTrieNode& node, bool sandBoxModex = false);
	void pruneTrieBelow(CTrieNode* node, CTrieNode* root, bool doNotDestroyLeaves, bool sandBoxModex = true, bool allowMultiThreaded = true);




	/**
	 * @brief Prunes a subtree of the trie below a specified node using a multi-threaded approach.
	 *
	 * This method performs a breadth-first traversal of the trie starting from the specified node,
	 * pruning (deleting) all nodes in the subtree except for the root node. It uses a thread pool
	 * to parallelize the pruning process, which can significantly speed up the operation for large tries.
	 *
	 * @details The pruning process involves the following steps:
	 * 1. Enqueue the starting node for processing.
	 * 2. Worker threads dequeue nodes and process them:
	 *    - For extension nodes: enqueue the next node, nullify the pointer to it in the parent, then delete the current node.
	 *    - For branch nodes: enqueue all non-null children, nullify pointers to them in the parent, then delete the current node.
	 *    - For leaf nodes: delete the node unless doNotDestroyLeaves is true.
	 * 3. Continue until all nodes in the subtree have been processed.
	 *
	 * The method ensures thread-safety through the use of mutex locks and atomic operations.
	 * It also handles exceptions that may occur during the pruning process.
	 *
	 * @param node Pointer to the node from which to start pruning. This node and its entire subtree
	 *             will be pruned, except if it's the root node.
	 * @param root Pointer to the root node of the trie. This node will not be deleted even if it's
	 *             the start node for pruning.
	 * @param doNotDestroyLeaves If true, leaf nodes will not be deleted during the pruning process.
	 * @param sandBoxMode Unused in this implementation. Originally intended to control deletion from cold storage.
	 * @param numThreads The number of worker threads to use for pruning. If a thread pool is provided,
	 *                   this parameter is ignored.
	 * @param threadPool Optional shared pointer to a ThreadPool. If provided, this thread pool will be
	 *                   used instead of creating a new one. If not provided, a new thread pool will be
	 *                   created with numThreads threads.
	 *
	 * @return This method does not return a value.
	 *
	 * @throws Rethrows any exceptions caught during the pruning process. These exceptions are caught
	 *         from individual worker threads and rethrown after all threads have completed.
	 *
	 * @note This method modifies the trie structure. After calling this method, all pointers to nodes
	 *       in the pruned subtree (except the root) should be considered invalid.
	 *
	 * @note The method uses the mPointerToPointerFromParent member of CTrieNode, which is a pointer
	 *       to the pointer in the parent node that points to the current node. This is nullified
	 *       before deleting a node to maintain the integrity of the trie structure.
	 *
	 * @note While this method is thread-safe internally, it's the caller's responsibility to ensure
	 *       that no other operations are performed on the affected part of the trie during pruning.
	 *
	 * @warning This operation is not reversible. Once a subtree is pruned, it cannot be recovered
	 *          unless you have external backups or references to the pruned nodes.
	 *
	 * @see CTrieNode, ThreadPool
	 */
	void CTrieDB::pruneTrieBelowMT(
		CTrieNode* node,
		CTrieNode* root,
		bool doNotDestroyLeaves,
		bool sandBoxMode = true,
		size_t numThreads = 8, // Optional argument for number of threads
		std::shared_ptr<ThreadPool> threadPool = nullptr
	);

	void pruneTrieMT(bool sandBoxMode = true, bool clearRootHashes = false, bool recalcPerspectiveP = true, uint64_t numThreads = 8, std::shared_ptr<ThreadPool> threadPool = nullptr);
	/**
	 * @brief Prunes the entire trie, with options for cold storage deletion, root hash clearing, and perspective recalculation.
	 *
	 * This method prunes the entire trie structure, starting from the root node. It uses either a multi-threaded
	 * or single-threaded approach based on compile-time settings, but always ensures thread-safety through locking.
	 *
	 * @details The pruning process involves the following steps:
	 * 1. Acquires an exclusive lock on the trie to ensure thread-safety.
	 * 2. Checks if the root node exists; if not, the method returns early.
	 * 3. Prunes the trie:
	 *    - If USE_MULTI_THREADED_TRIE_PRUNING is defined:
	 *      Calls pruneTrieMT with the provided parameters and the number of CPU cores.
	 *    - Otherwise:
	 *      Calls pruneTrieBelow to recursively delete all nodes except the root.
	 * 4. If clearRootHashes is true, clears the hashes of the root node.
	 * 5. If recalcPerspectiveP is true, recalculates the trie's perspective (root hash).
	 *
	 * @param sandBoxMode If false, removes data from both hot storage (RAM) and cold storage (HDD/SSD).
	 *                    If true, only affects data in hot storage.
	 * @param clearRootHashes If true, clears the hashes of the root node after pruning.
	 * @param recalcPerspectiveP If true, recalculates the hash of the Merkle Patricia Trie root after pruning.
	 *
	 * @return This method does not return a value.
	 *
	 * @note The method is thread-safe due to the use of an exclusive lock.
	 *
	 * @note The trie perspective refers to the hash of the Merkle Patricia Trie root. Recalculating
	 *       this hash ensures that the trie's state is accurately reflected after pruning.
	 *
	 * @warning This operation modifies the entire trie structure and is not reversible. Ensure
	 *          you have necessary backups or references if you need to preserve any data.
	 *
	 * @warning When sandBoxMode is false, data will be removed from cold storage. This operation
	 *          cannot be undone, so use with caution.
	 *
	 * @todo Validate the necessity of recalculating perspective when the object is about to be destroyed,
	 *       and ensure proper memory management for any allocations made during recalcPerspective().
	 *
	 * @see pruneTrieBelow, pruneTrieMT, recalcPerspective, CTrieBranchNode
	 */
	void pruneTrie(bool sandBoxMode = true, bool clearRootHashes = false, bool recalcPerspective = true, bool allowMultiThreaded = true);
	bool testTrie(size_t& nodeCount, bool doActivePruning = true, bool pruneAfter = true, bool flashCurrentCount = false, eTrieSize::eTrieSize trieSize = eTrieSize::mediumTrie, uint64_t maxNOfNodesToTest = 0);
	bool testTrieMT(size_t& nodeCount, bool doActivePruning = true, bool pruneAfter = true, bool flashCurrentCount = false, uint64_t maxNOfNodesToTest = 0, uint64_t numThreads = 8, std::shared_ptr<ThreadPool> threadPool = nullptr);
	void getHotStorageLeaves(std::vector<CTrieNode*>& leaves);
	std::vector<std::vector<uint8_t>> getKnownNodeIDs();

	//top-bottom

	std::vector<uint8_t> getPackedMerklePathBottomTop(CTrieLeafNode* leaf);
	std::vector<uint8_t> getPackedMerklePathTopBottom(std::vector<uint8_t> id);
	std::vector<uint8_t> getPackedMerklePathTopBottom(std::vector<nibblePair> path); //takes into account the lastest root
	bool isRootEmptyHS();
	bool verifyPackedMerklePath(std::vector<uint8_t> proof, std::vector<uint8_t> rootHash = std::vector<uint8_t>(), bool introduceRandomError = false);
	bool findNodesWithName(std::regex& regex, std::vector<std::string>& matchingPaths, BigInt ERGLimit, BigInt& ERGUsed, std::string currentPath = "");
	std::vector<CTrieNode*> getAllSubLeafNodes(eTrieSize::eTrieSize trieSize = eTrieSize::mediumTrie, bool useColdStorage = true, bool allowMT = true);

	/*
	# Multi-threaded Trie Traversal Algorithm

	## Purpose
	This algorithm performs a multi-threaded traversal of a trie data structure to efficiently find and report all leaf nodes. It is designed to handle large trie structures by leveraging parallel processing, making it particularly useful for applications dealing with extensive datasets or requiring high-performance trie operations.

	## Parameters
	- `root`: Pointer to the root node of the trie.
	- `node`: Pointer to the starting node for traversal (can be the root or a subnode).
	- `path`: Initial path to the starting node.
	- `useColdStorage`: Boolean flag indicating whether to use cold storage functionality.
	- `numThreads`: Number of worker threads to spawn for parallel processing.

	## Return Value
	A vector of pointers to all leaf nodes found during the traversal.

	## Algorithm Overview

	### 1. Initialization
	- Create a shared task queue for nodes to be processed.
	- Initialize a vector to store found leaf nodes.
	- Set up synchronization primitives (mutex, condition variable).
	- Initialize an atomic counter for active tasks.

	### 2. Worker Thread Creation
	- Spawn `numThreads` worker threads to process nodes concurrently.

	### 3. Breadth-First Traversal
	- Start with the given node (root or subnode) and enqueue it.
	- Worker threads dequeue and process nodes:
	  - Empty nodes are skipped.
	  - Extension nodes enqueue their next node.
	  - Branch nodes enqueue all non-null children.
	  - Leaf nodes are added to the result vector.

	### 4. Cold Storage Handling
	- If `useColdStorage` is true, validate node availability before processing.

	### 5. Path Tracking
	- Maintain and update the path to each node during traversal.

	### 6. Concurrent Processing
	- Multiple threads work on different branches of the trie simultaneously.
	- Use synchronization primitives to ensure thread-safe access to shared resources.

	### 7. Termination
	- Algorithm completes when all nodes have been processed.
	- Use atomic counter and empty queue condition to determine completion.

	### 8. Result Collection
	- Return the vector of all discovered leaf nodes.

	## Key Components

	### Task Queue
	- Stores nodes (tasks) to be processed.
	- Thread-safe operations for enqueueing and dequeueing.

	### Active Tasks Counter
	- Atomic counter tracking tasks in queue or being processed.
	- Used for determining algorithm completion.

	### Condition Variable
	- Allows threads to wait efficiently for new tasks or termination signal.

	### Worker Function
	- Main logic for processing nodes.
	- Handles different node types and enqueues child nodes.

	## Thread Synchronization

	### Mutex Usage
	- Protects shared resources (task queue, result vector).
	- Ensures atomic operations on the task queue.

	### Condition Variable Signaling
	- `notify_one()`: Wake up one thread when a new task is added.
	- `notify_all()`: Wake up all threads for potential termination.

	### Wait Mechanism
	```cpp
	queueCV.wait(lock, [&]() {
		return !taskQueue.empty() || activeTasks.load(std::memory_order_acquire) == 0;
	});
	```
	- Efficiently wait for new tasks or termination condition.
	- Prevents spurious wake-ups.

	## Termination Logic
	- Algorithm terminates when:
	  1. Task queue is empty.
	  2. Active tasks counter reaches zero.
	- All threads exit their processing loop upon meeting these conditions.

	## Cold Storage Consideration
	- `validateNextNodeAvailability()` function called when `useColdStorage` is true.
	- Ensures nodes are loaded into memory before processing.

	## Exception Handling
	- Catches and stores exceptions thrown during processing.
	- Signals all threads to exit if an exception occurs.

	## Performance Considerations
	1. Load Balancing: Automatic load distribution among threads.
	2. Scalability: Performance scales with the number of threads and CPU cores.
	3. Memory Efficiency: Processes nodes on-demand, suitable for large tries.

	## Usage Example
	```cpp
	CTrieDB trieDB;
	CTrieNode* root = trieDB.getRoot();
	std::vector<nibblePair> initialPath;
	bool useColdStorage = false;
	size_t numThreads = std::thread::hardware_concurrency();

	auto leafNodes = trieDB.getAllSubLeafNodesMT(root, root, initialPath, useColdStorage, numThreads);
	```

	## Advantages
	1. Parallel Processing: Efficiently handles large tries.
	2. Flexible Starting Point: Can start traversal from any node.
	3. Cold Storage Support: Adaptable to different storage mechanisms.
	4. Scalable: Performance improves with more CPU cores.

	## Limitations and Considerations
	1. Thread Overhead: For small tries, thread creation overhead might outweigh benefits.
	2. Memory Usage: Concurrent processing may increase memory usage.
	3. Cold Storage Performance: Heavy use of cold storage may impact performance.

	## Possible Optimizations
	1. Thread Pool: Reuse threads to reduce creation overhead.
	2. Work Stealing: Implement work-stealing for better load balancing.
	3. Batched Processing: Group small tasks to reduce synchronization overhead.

	## Conclusion
	This multi-threaded trie traversal algorithm provides an efficient solution for processing large trie structures. By leveraging parallel processing and careful synchronization, it offers significant performance benefits over single-threaded approaches, especially for extensive datasets or high-performance requirements.
	*/
	std::vector<CTrieNode*> CTrieDB::getAllSubLeafNodesMT(
		CTrieNode* root,
		CTrieNode* node,
		std::vector<nibblePair> path = std::vector<nibblePair>(),
		bool useColdStorage = true,
		size_t numThreads = 8,
		std::shared_ptr<ThreadPool> externalPool = nullptr
	);
	std::vector<CTrieNode*> getAllSubLeafNodes(CTrieNode* root, CTrieNode* node, std::vector<nibblePair> path = std::vector<nibblePair>(), bool useColdStorage = true);
	//TODO:add function taking into accunt ROOT; there might be multiple roots for same leaf (will be)
	CTrieNode* findNodeByFullID(std::vector<uint8_t> ID, bool disableIntegrityChecks = false, bool disablePerspectiveUpdate = false);
	CTrieNode* findNodeByFullID(std::vector<nibblePair> path, bool disableIntegrityChecks = false, bool disablePerspectiveUpdate = false);	//should take into account the latest root
	CTrieNode* getRoot();
	CTrieDB::eResult addNodeExt(std::vector<nibblePair> id, CTrieNode* toAdd, bool sandBoxModex = false, bool disableIntegrityChecks = false, bool justUpdateSecToken = false);
	CTrieDB::eResult addNode(std::vector<nibblePair> id, CTrieNode* toAdd, bool sandBoxModex = false, bool disableIntegrityChecks = false, bool justSetSecToken = false);
	CTrieNode* addNodeGetRoot(std::vector<nibblePair> idPart, CTrieNode* root,
		CTrieNode* current, CTrieNode* toAdd, bool sandBoxModex = false);
	bool verifyTriePath(std::vector<uint8_t> rootHash, CTrieNode* child);
	bool verifyDBTriePath(CTrieNode* child);

	// ACID Domain Log - BEGIN
	bool registerStateDomainsInitialHash(std::vector<uint8_t> domainAddress, std::vector<uint8_t> hash);
	std::vector<std::vector<uint8_t>> getStateDomainsModifiedDuringTheFlow();
	bool getStateDomainInitialHash(std::vector<uint8_t> domainAddress, std::vector<uint8_t>& hash);
	// ACID Domain Log - END

};

#endif
