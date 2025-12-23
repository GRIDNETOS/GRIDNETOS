#pragma once
#include <vector>

struct cpFlags
{
	bool  active: 1;
	bool  obligatory : 1;
	bool reserved : 6;
	cpFlags(const cpFlags& sibling) {
		std::memcpy(this, &sibling, sizeof(cpFlags));
	}

	cpFlags()
	{
		std::memset(this, 0, sizeof(cpFlags));
	}
};
/*
* [Checkpoints are introduced ONLY when]:
* 1) Due to changes in code, we are unable to arrive at the very same Perspective as we used to in the past.
* 
* In such a case we introduce a new checkpoint.
* 
* [Checkpoint are of two types]:
* 1) Those having explicit Perspective specified.
* 2) Those without an explicit Perspective set.
* 
* [Things to keep in mind]:
* 
* [Most Important/Limitations]: when the history of events takes a certain turn at point of time T1 while processed by software version S1, there is no way for 
* software version S2 to take the same turn IF it was not made 100% compatible with S1 (in terms of the processing logic of transactions with S2). 
  In such a case, a checkpoint would ONLY help S2 to accept a segment of blocks but the resulting perspective
* WOULD be DIFFERENT. Also, the checkpoint would in no way help S1 (if still running on some nodes) arrive at same perspectives, with consecutive blocks, as S2 does.
* That holds true under the strength of one way hash transformation since that would require a collision.
* 
* To put things into perspective, if node S1 at block-height N kept trying to validate new blocks and if those blocks at N+1 were incompatible with 1
* in terms of the Final Effective Perspective,and if not covered through a checkpoint (from the S1's perspective) - S1 would be unable to verify these blocks forever.
* S1 would need to update its version to S2 in order to proceed any further. It might be compared to mixing of colors analogy. Imagine that S1 kept mixing colors for very long,
* with each block representing a unique color, while with each mix adding a certain secret ingredient color. Now, S2 in its newer version kept adding another secret color with each color
* mixture operation. The resulting  color, after all the mixture operations would thus never be the same. The secret color is the difference in processing logic (resulting is possibly even
* slightly different data, thus in different root of the merkle patricia trie).
* 
* [The Gist] the main purpose of Checkpoints is thus to prevent the need for wiping out of prior data (the entire history of events) if for any reason should S2
* be unable to arrive at same Effective Perspective as S1 did. Checkpoints do would not help S1 in any way.
 ===> In the face of such an event (when a new checkpoint needed to be added into the System) S1 would need to upgrade to S2 and **reprocess the ENTIRE history of events**.
 The winning part of all this is that we may continue upgrading the processing logic and data structures without ever wiping out the prior history of events.
 The property of immutability of data representation of the prior history of events stemming back to Genesis Block is thus prioritized above everything in terms of backwards' compatibility
 support.
* 


* 1) A checkpoint does NOT prevent contents of Blocks and Transactions from being processed and/or evaluated.
*	 An invalid TX would have failed whether the Block encapsulating it is 'covered' through a Checkpoint or not.
* 2) A checkpoint ONLY overrides verification of the Perspective resulting from processing of an entire Block.
* 3) Thus, all the TXs, Receipts, Verifiables are processed in accordance to the latest code-base.
* 4) For the above to be possible, there needs to be a data-translation layer (for TXs , Receipts etc.) which would translate
*	 previous versions of the containers into their latest revisions.
* IMPORTANT: containers (TXs, Receipts etc.) are always serialized into their ORIGINAL version. We use mOriginalVersion to keep track of the original data-structure
* revision. Whenever serialized, the original version identifier is put into place. Also, whenever computing an image of the container,
* it is serialized in accordance to its original revision. For TXs pay attention to CTransaction::getConcatData().
* 
* 5) Whenever a change to the Software causing a discrepancy in Perspective is introduced, it would render all current nodes unable to arrive at it. Forever.
*	That is because they would keep building upon a stale version of the main Merkle-Patricia-Trie. Letting Blocks through 'thanks to' a  checkpoint
*	would not made the data-integrity footprint (current Perspective) match with what is expected at this very checkpoint.
*	
*	Thus, all nodes, in order to move past the last checkpoint - they need to re-validate the *entire* history of events.
*	The system would detect such an event autonomously if the corresponding checkpoint has a an explicit Perspective specified.
*	Then, if the corresponding Perspective is not reached at the given Checkpoint - the system would clear the State (mVerifiedChainProof, mLeader etc.).
*	and the processing would commence from the Genesis Block.
* 
*/
class CBCheckpoint
{
private:
	cpFlags mFlags;
	std::mutex mGuardian;
	bool mIsObligatory;
	size_t mHeight;
	std::vector<uint8_t> mHash; //block's image
	std::vector<uint8_t> mPerspective;//the Perspective which is required to the moment processing of a Block represented by the Checkpoint is finished.
public:
	void setIsActive(bool isIt = true);
	bool getIsActive();
	cpFlags getFlags();
	void setFlags(cpFlags flags);
	CBCheckpoint(cpFlags flags,size_t height,const std::vector<uint8_t> & blockImage , const std::vector<uint8_t> &pespective = std::vector<uint8_t>());
	size_t getHeight();
	std::vector<uint8_t> getHash();
	std::vector<uint8_t> getPerspective();

};