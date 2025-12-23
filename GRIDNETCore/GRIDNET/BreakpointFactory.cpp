#include "BreakpointFactory.hpp"
#include <algorithm>


CBreakpointFactory::CBreakpointFactory(std::shared_ptr<CTools> tools) {
    mTools = tools;
}

bool CBreakpointFactory::addBreakpoint(std::shared_ptr<CBreakpoint> breakpoint) {
    if (!breakpoint) return false;

    std::unique_lock<std::shared_mutex> lock(mMutex);

    // Check if breakpoint already exists
    auto it = std::find(mBreakpoints.begin(), mBreakpoints.end(), breakpoint);
    if (it != mBreakpoints.end()) {
        return false;
    }

    // Add to main container
    mBreakpoints.push_back(breakpoint);

    // Update indexes
    updateIndexes(breakpoint);

    return true;
}

bool CBreakpointFactory::removeBreakpoint(std::shared_ptr<CBreakpoint> breakpoint) {
    if (!breakpoint) return false;

    std::unique_lock<std::shared_mutex> lock(mMutex);

    auto it = std::find(mBreakpoints.begin(), mBreakpoints.end(), breakpoint);
    if (it == mBreakpoints.end()) {
        return false;
    }

    // Remove from indexes first
    removeFromIndexes(breakpoint);

    // Remove from main container
    mBreakpoints.erase(it);

    return true;
}

void CBreakpointFactory::updateIndexes(const std::shared_ptr<CBreakpoint>& breakpoint) {
    switch (breakpoint->getType()) {
    case eBreakpointType::block:
        switch (breakpoint->getCondition()) {
        case eBreakpointCondition::height:
            mHeightIndex.emplace(std::stoull(breakpoint->getValueString()), breakpoint);
            break;
        case eBreakpointCondition::keyHeight:
            mKeyHeightIndex.emplace(std::stoull(breakpoint->getValueString()), breakpoint);
            break;
        case eBreakpointCondition::blockID:
            mBlockIDIndex.emplace(breakpoint->getValueBytes(), breakpoint);
            break;
        default:
            break;
        }
        break;

    case eBreakpointType::transaction:
        switch (breakpoint->getCondition()) {
        case eBreakpointCondition::receiptID:
            mReceiptIDIndex.emplace(breakpoint->getValueBytes(), breakpoint);
            break;
        case eBreakpointCondition::txSource:
            mSourceIndex.emplace(breakpoint->getValueString(), breakpoint);
            break;
        case eBreakpointCondition::txDestination:
            mDestinationIndex.emplace(breakpoint->getValueString(), breakpoint);
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
}

void CBreakpointFactory::removeFromIndexes(const std::shared_ptr<CBreakpoint>& breakpoint) {
    switch (breakpoint->getType()) {
    case eBreakpointType::block:
        switch (breakpoint->getCondition()) {
        case eBreakpointCondition::height: {
            auto height = std::stoull(breakpoint->getValueString());
            auto range = mHeightIndex.equal_range(height);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == breakpoint) {
                    mHeightIndex.erase(it);
                    break;
                }
            }
            break;
        }
        case eBreakpointCondition::keyHeight: {
            auto keyHeight = std::stoull(breakpoint->getValueString());
            auto range = mKeyHeightIndex.equal_range(keyHeight);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == breakpoint) {
                    mKeyHeightIndex.erase(it);
                    break;
                }
            }
            break;
        }
        case eBreakpointCondition::blockID: {
            auto range = mBlockIDIndex.equal_range(breakpoint->getValueBytes());
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == breakpoint) {
                    mBlockIDIndex.erase(it);
                    break;
                }
            }
            break;
        }
        default:
            break;
        }
        break;

    case eBreakpointType::transaction:
        switch (breakpoint->getCondition()) {
        case eBreakpointCondition::receiptID: {
            auto range = mReceiptIDIndex.equal_range(breakpoint->getValueBytes());
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == breakpoint) {
                    mReceiptIDIndex.erase(it);
                    break;
                }
            }
            break;
        }
        case eBreakpointCondition::txSource: {
            auto range = mSourceIndex.equal_range(breakpoint->getValueString());
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == breakpoint) {
                    mSourceIndex.erase(it);
                    break;
                }
            }
            break;
        }
        case eBreakpointCondition::txDestination: {
            auto range = mDestinationIndex.equal_range(breakpoint->getValueString());
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == breakpoint) {
                    mDestinationIndex.erase(it);
                    break;
                }
            }
            break;
        }
        default:
            break;
        }
        break;

    default:
        break;
    }
}

// Retrieval methods implementation
std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getBreakpointsAtHeight(uint64_t height) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::vector<std::shared_ptr<CBreakpoint>> results;
    auto range = mHeightIndex.equal_range(height);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    return results;
}

std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getBreakpointsAtKeyHeight(uint64_t keyHeight) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::vector<std::shared_ptr<CBreakpoint>> results;
    auto range = mKeyHeightIndex.equal_range(keyHeight);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    return results;
}


std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getBreakpointsForBlock(const std::vector<uint8_t>& blockID) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::vector<std::shared_ptr<CBreakpoint>> results;
    auto range = mBlockIDIndex.equal_range(blockID);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    return results;
}

std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getBreakpointsForReceipt(const std::vector<uint8_t>& receiptID) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::vector<std::shared_ptr<CBreakpoint>> results;
    auto range = mReceiptIDIndex.equal_range(receiptID);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    return results;
}

std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getBreakpointsForSource(const std::string& source) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::vector<std::shared_ptr<CBreakpoint>> results;
    auto range = mSourceIndex.equal_range(source);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    return results;
}

std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getBreakpointsForDestination(const std::string& destination) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::vector<std::shared_ptr<CBreakpoint>> results;
    auto range = mDestinationIndex.equal_range(destination);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    return results;
}

uint64_t CBreakpointFactory::getBreakpoinstCount(bool onlyActive) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if (!onlyActive) {
        return mBreakpoints.size();
    }

    return std::count_if(mBreakpoints.begin(), mBreakpoints.end(),
        [](const auto& bp) {
            return bp->isActive();
        });
}

bool CBreakpointFactory::getHasBreakpoints()
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return !mBreakpoints.empty();
}

uint64_t CBreakpointFactory::getTransactionBreakpointCount(bool onlyActive) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if (mBreakpoints.empty())
    {
        return 0;
    }

    return std::count_if(mBreakpoints.begin(), mBreakpoints.end(),
        [onlyActive](const auto& bp) {
            return bp->getType() == eBreakpointType::transaction &&
                (!onlyActive || bp->isActive());
        });
}



uint64_t CBreakpointFactory::getBlockBreakpointCount(bool onlyActive ) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    if (mBreakpoints.empty())
    {
        return 0;
    }
    return std::count_if(mBreakpoints.begin(), mBreakpoints.end(),
        [onlyActive](const auto& bp) {
            return bp->getType() == eBreakpointType::block &&
                (!onlyActive || bp->isActive());
        });
}

void CBreakpointFactory::activateAllBreakpoints() {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& breakpoint : mBreakpoints) {
        breakpoint->activate();
    }
}


/**
    * @brief Gets all breakpoints in specified state for an object
    *
    * @param objectID Block ID or receipt ID
    * @param state Required state, or none for any non-none state
    * @return std::vector<std::shared_ptr<CBreakpoint>> Matching breakpoints
    */
std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getActiveBreakpoints(
    const std::vector<uint8_t>& objectID,
    eBreakpointState::eBreakpointState state = eBreakpointState::none) const
{

    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::vector<std::shared_ptr<CBreakpoint>> result;

    for (const auto& bp : mBreakpoints) {
        if (bp->isActive() && mTools->compareByteVectors(bp->getObjectID(), objectID)) {
            if (state == eBreakpointState::none || bp->getState() == state) {
                result.push_back(bp);
            }
        }
    }

    return result;
}

/**
 * @brief Sets state for matching breakpoints for an object
 *
 * @param breakpoints Vector of breakpoints to update
 * @param state New state to set
 * @param objectID Block ID or receipt ID being processed
 */
void CBreakpointFactory::setState(
    const std::vector<std::shared_ptr<CBreakpoint>>& breakpoints,
    eBreakpointState::eBreakpointState state,
    const std::vector<uint8_t>& objectID)
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& bp : breakpoints) {
        bp->setState(state, objectID);
    }
}

/**
 * @brief Clears state for all breakpoints of an object
 *
 * @param objectID Block ID or receipt ID
 */
void CBreakpointFactory::clearState(const std::vector<uint8_t>& objectID)
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& bp : mBreakpoints) {
        if (bp->isActive() && mTools->compareByteVectors(bp->getObjectID(), objectID)) {
            bp->clear();
        }
    }
}

/**
 * @brief Checks if any breakpoints are active for an object
 *
 * @param objectID Block ID or receipt ID
 * @param state Required state, or none for any non-none state
 * @return bool True if any matching breakpoints exist
 */
bool CBreakpointFactory::hasActiveBreakpoints(
    const std::vector<uint8_t>& objectID,
    eBreakpointState::eBreakpointState state = eBreakpointState::none) const
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& bp : mBreakpoints) {
        if (bp->isActive() && mTools->compareByteVectors(bp->getObjectID(), objectID)) {
            if (state == eBreakpointState::none || bp->getState() == state) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Get count of active breakpoints for an object
 *
 * @param objectID Block ID or receipt ID
 * @param state Required state, or none for any non-none state
 * @return uint64_t Number of matching breakpoints
 */
uint64_t CBreakpointFactory::getActiveBreakpointCount(
    const std::vector<uint8_t>& objectID,
    eBreakpointState::eBreakpointState state = eBreakpointState::none) const
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return std::count_if(mBreakpoints.begin(), mBreakpoints.end(),
        [&](const auto& bp) {
            return bp->isActive() && mTools->compareByteVectors(bp->getObjectID(), objectID) &&
                (state == eBreakpointState::none || bp->getState() == state);
        });
}
void CBreakpointFactory::activateAllBlockBreakpoints() {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& breakpoint : mBreakpoints) {
        if (breakpoint->getType() == eBreakpointType::block) {
            breakpoint->activate();
        }
    }
}

void CBreakpointFactory::activateAllTransactionBreakpoints() {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& breakpoint : mBreakpoints) {
        if (breakpoint->getType() == eBreakpointType::transaction) {
            breakpoint->activate();
        }
    }
}

std::vector<std::shared_ptr<CBreakpoint>> CBreakpointFactory::getAllBreakpoints() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mBreakpoints;
}

void CBreakpointFactory::deactivateAllBreakpoints() {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& breakpoint : mBreakpoints) {
        breakpoint->deactivate();
    }
}

void CBreakpointFactory::deactivateAllBlockBreakpoints() {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& breakpoint : mBreakpoints) {
        if (breakpoint->getType() == eBreakpointType::block) {
            breakpoint->deactivate();
        }
    }
}

void CBreakpointFactory::deactivateAllTransactionBreakpoints() {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    for (const auto& breakpoint : mBreakpoints) {
        if (breakpoint->getType() == eBreakpointType::transaction) {
            breakpoint->deactivate();
        }
    }
}