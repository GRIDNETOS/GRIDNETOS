#include "StatusBarHub.h"
#include "Tools.h"
std::mutex CStatusBarHub::sInstanceGuardian;
std::shared_ptr< CStatusBarHub> CStatusBarHub::sInstance;

uint64_t CStatusBarHub::getNextCustomStatusBarID(eBlockchainMode::eBlockchainMode mode) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return ++nextIDs[mode];
}

void CStatusBarHub::cleanStatusBars(eBlockchainMode::eBlockchainMode mode) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    uint64_t currentTime = std::time(0);
    auto& bars = mCustomStatusBars[mode];
    for (auto it = bars.rbegin(); it != bars.rend(); /*no increment here*/) {
        uint64_t timestamp = std::get<1>(it->second);
        uint64_t minDisplayTime = std::get<2>(it->second);
        uint64_t age = (currentTime> timestamp) ?(currentTime - timestamp):0;
        if (age > 180) {
            it = std::map<uint64_t, std::tuple<std::string, uint64_t, uint64_t>>::reverse_iterator(bars.erase(std::next(it).base()));
        }
        else {
            ++it;
        }
    }
}

uint64_t CStatusBarHub::getCustomStatusBarShownTimestamp(eBlockchainMode::eBlockchainMode mode) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    if (mCustomStatusBars[mode].find(currentCustomBarIndices[mode]) != mCustomStatusBars[mode].end()) {
        // Access the timestamp using std::get<1>
        return std::get<1>(mCustomStatusBars[mode][currentCustomBarIndices[mode]]);
    }
    return 0;
}


CStatusBarHub::CStatusBarHub()
{
    mMinDisplayTime = 5;
}

uint64_t CStatusBarHub::getDefaultDisplayTime()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return  mMinDisplayTime;
}

void CStatusBarHub::setDefaultDisplayTime(uint64_t time)
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    mMinDisplayTime = time;
}

std::shared_ptr<CStatusBarHub> CStatusBarHub::getInstance() {
    std::lock_guard<std::mutex> lock(sInstanceGuardian);
    if (!sInstance) {  // Check if instance doesn't exist.
        sInstance = std::make_shared<CStatusBarHub>();  // Create instance.
    }
    return sInstance;  // Return the instance.
}



bool CStatusBarHub::removeCustomStatusBarByID(eBlockchainMode::eBlockchainMode mode, uint64_t id) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    auto& bars = mCustomStatusBars[mode];
    if (bars.find(id) != bars.end()) {
        bars.erase(id);
        return true;
    }
    return false;
}

std::string CStatusBarHub::getCustomStatusBarText(eBlockchainMode::eBlockchainMode mode, uint64_t& timestamp) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);

    // NOTICE: mCustomStatusBars maps blockchain modes to BAR IDENTIFIERS **NOT** to their indexes. 
    if (mCustomStatusBars[mode].find(currentCustomBarIndices[mode]) != mCustomStatusBars[mode].end()) {
        // Access the timestamp and text using std::get<>() from the tuple
        timestamp = std::get<1>(mCustomStatusBars[mode][currentCustomBarIndices[mode]]); // For timestamp
        return std::get<0>(mCustomStatusBars[mode][currentCustomBarIndices[mode]]); // For text
    }
    timestamp = 0;
    return "";  // Return empty string if not found
}


void CStatusBarHub::setCustomStatusBarText(eBlockchainMode::eBlockchainMode mode, uint64_t id, const std::string& txt, uint64_t minDisplayTime) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    auto currentTime = mTools->getTime();
    mCustomStatusBars[mode][id] = std::make_tuple(txt, currentTime, minDisplayTime);
}

void CStatusBarHub::invalidateCustomStatusBar(eBlockchainMode::eBlockchainMode mode, uint64_t id) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    if (mCustomStatusBars[mode].find(id) != mCustomStatusBars[mode].end()) {
        // Reset the timestamp to 0 using std::get<1> for the second element of the tuple
        std::get<1>(mCustomStatusBars[mode][id]) = 0;
    }
    // Optionally, handle the scenario where 'id' doesn't exist for the provided 'mode'
}

std::map<uint64_t, std::tuple<std::string, uint64_t, uint64_t>> CStatusBarHub::getCustomStatusBars(eBlockchainMode::eBlockchainMode mode) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    // Directly return the map for the specified mode
    return mCustomStatusBars[mode];
}

uint64_t CStatusBarHub::getCurrentCustomBarIndex(eBlockchainMode::eBlockchainMode mode) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    if (currentCustomBarIndices.find(mode) != currentCustomBarIndices.end()) {
        return currentCustomBarIndices[mode];
    }
    return 0;  // Default value if 'mode' doesn't exist
}

void CStatusBarHub::setCurrentCustomBarIndex(eBlockchainMode::eBlockchainMode mode, uint64_t index) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    currentCustomBarIndices[mode] = index;
}
