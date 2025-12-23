#include "Breakpoint.hpp"

CBreakpoint::CBreakpoint()
    : mType(eBreakpointType::code)
    , mCondition(eBreakpointCondition::none)
    , mActive(false)
    , mHitCount(0)
    , mLine(0)
{
}

CBreakpoint::CBreakpoint(uint64_t line,bool active)
    : mType(eBreakpointType::code)
    , mCondition(eBreakpointCondition::none)
    , mActive(active)
    , mHitCount(0)
    , mLine(line)
{
}

CBreakpoint::CBreakpoint(eBreakpointCondition::eBreakpointCondition condition,
    const std::string& value, bool active)
    : mType(eBreakpointType::block)
    , mCondition(condition)
    , mActive(active)
    , mHitCount(0)
    , mLine(0)
    , mValueString(value)
{
    validateBlockBreakpoint(condition,value);
}

CBreakpoint::CBreakpoint(eBreakpointCondition::eBreakpointCondition condition,
    const std::vector<uint8_t>& value, bool active)
    : mType(eBreakpointType::transaction)
    , mCondition(condition)
    , mActive(active)
    , mHitCount(0)
    , mLine(0)
    , mValueBytes(value)
{
    validateBlockBreakpoint(condition, std::string(value.begin(), value.end()));
}

void CBreakpoint::activate() {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mActive = true;
}

void CBreakpoint::deactivate() {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mActive = false;
}

bool CBreakpoint::isActive() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mActive;
}

bool CBreakpoint::validateBlockBreakpoint(eBreakpointCondition::eBreakpointCondition condition,
    const std::string& value)
{
    std::shared_ptr<CTools> tools = CTools::getInstance();

    // Validate condition type
    if (condition != eBreakpointCondition::height &&
        condition != eBreakpointCondition::keyHeight &&
       condition != eBreakpointCondition::blockID) {
        return true;
        //throw std::invalid_argument("Invalid condition type for block breakpoint");
    }

    // Validate value based on condition
    switch (condition) {
    case eBreakpointCondition::blockID: {
        std::vector<uint8_t> blockID(value.begin(), value.end());
        if (!tools->isBlockIDValid(blockID)) {
            return true;
         //   throw std::invalid_argument("Invalid block ID format");
        }
        break;
    }

    case eBreakpointCondition::height:
    case eBreakpointCondition::keyHeight: {
        if (!tools->is_number(value)) {
           // throw std::invalid_argument("Height must be a valid number");
        }
        uint64_t height;
        if (!tools->stringToUint(value, height)) {
            return true;
           // throw std::invalid_argument("Invalid height value");
        }
        break;
    }

    default:
        return false;
        //throw std::invalid_argument("Unsupported block breakpoint condition");
    }
    return false;
}



void CBreakpoint::hit(eBreakpointState::eBreakpointState state) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    ++mHitCount;
    if (state != eBreakpointState::none)
    {
        mState = state;
    }
}

uint64_t CBreakpoint::getHitCount() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mHitCount;
}

eBreakpointType::eBreakpointType CBreakpoint::getType() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mType;
}

eBreakpointCondition::eBreakpointCondition CBreakpoint::getCondition() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mCondition;
}

uint64_t CBreakpoint::getLine() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mLine;
}



std::string CBreakpoint::getValueString() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mValueString;
}

std::vector<uint8_t> CBreakpoint::getValueBytes() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mValueBytes;
}

/**
    * @brief Gets the current state of the breakpoint
    * @return eBreakpointState::eBreakpointState Current execution state
    */
eBreakpointState::eBreakpointState CBreakpoint::getState() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mState;
}


/**
    * @brief Checks if breakpoint is currently processing a specific object
    *
    * @param objectID Block ID or receipt ID to check
    * @return bool True if this breakpoint is processing the specified object
    */
bool CBreakpoint::isProcessing(const std::vector<uint8_t>& objectID) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mState != eBreakpointState::none && mAtatchedToObjectID.empty();
}

/**
 * @brief Sets the state of the breakpoint
 * @param state New execution state
 */

void CBreakpoint::setState(eBreakpointState::eBreakpointState state, const std::vector<uint8_t>& objectID) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mState = state;
    mAtatchedToObjectID = objectID;
}

/**
  * @brief Gets the active object ID (if any)
  * @return std::vector<uint8_t> Current block ID or receipt ID being processed
  */
std::vector<uint8_t> CBreakpoint::getObjectID() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mAtatchedToObjectID;
}


/**
    * @brief Clears the processing state
    */
void CBreakpoint::clear() {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mState = eBreakpointState::none;
    mAtatchedToObjectID.clear();
    mPreExecutionPerspective.clear();
}


void CBreakpoint::setValue(const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    if (mType == eBreakpointType::block &&
        (mCondition == eBreakpointCondition::height ||
            mCondition == eBreakpointCondition::keyHeight ||
            mCondition == eBreakpointCondition::blockID)) {
        mValueString = value;
    }
    else {
        throw std::invalid_argument("Cannot set string value for this breakpoint type/condition");
    }
}

void CBreakpoint::setValue(const std::vector<uint8_t>& value) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    if (mType == eBreakpointType::transaction &&
        (mCondition == eBreakpointCondition::receiptID ||
            mCondition == eBreakpointCondition::txSource ||
            mCondition == eBreakpointCondition::txDestination)) {
        mValueBytes = value;
    }
    else {
        throw std::invalid_argument("Cannot set byte value for this breakpoint type/condition");
    }
}

std::vector<uint8_t> CBreakpoint::getPreExecutionPerspective()
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mPreExecutionPerspective;
}

void CBreakpoint::setPreExecutionPerspective(const std::vector<uint8_t>& perspective)
{
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mPreExecutionPerspective = perspective;
}
