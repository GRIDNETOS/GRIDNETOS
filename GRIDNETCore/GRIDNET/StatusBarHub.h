#pragma once
#include <mutex>
class CStatusBarHub
{
private:

   
    static std::mutex sInstanceGuardian;
    std::map<eBlockchainMode::eBlockchainMode, std::map<uint64_t, std::tuple<std::string, uint64_t, uint64_t>>> mCustomStatusBars;
    std::mutex mFieldsGuardian;
     std::shared_ptr<CTools> mTools;
     std::map<eBlockchainMode::eBlockchainMode, uint64_t> currentCustomBarIndices;
    std::map<eBlockchainMode::eBlockchainMode, uint64_t> nextIDs;
    static std::shared_ptr< CStatusBarHub> sInstance;
    uint64_t mMinDisplayTime;
    ///uint64_t mCurrentCustomBarID; //NO! shared through DTIs and local console
public:

    CStatusBarHub();
    ~CStatusBarHub() = default;
    CStatusBarHub(const CStatusBarHub&) = delete;
    CStatusBarHub& operator=(const CStatusBarHub&) = delete;
    uint64_t getDefaultDisplayTime();
    void setDefaultDisplayTime(uint64_t time);

    static std::shared_ptr<CStatusBarHub> getInstance();
    uint64_t getNextCustomStatusBarID(eBlockchainMode::eBlockchainMode mode);
    void cleanStatusBars(eBlockchainMode::eBlockchainMode mode);
    uint64_t getCustomStatusBarShownTimestamp(eBlockchainMode::eBlockchainMode mode);
    bool removeCustomStatusBarByID(eBlockchainMode::eBlockchainMode mode, uint64_t id);
    std::string getCustomStatusBarText(eBlockchainMode::eBlockchainMode mode, uint64_t& timestamp);
    void setCustomStatusBarText(eBlockchainMode::eBlockchainMode mode, uint64_t id, const std::string& txt, uint64_t minDisplayTime=5);
    void invalidateCustomStatusBar(eBlockchainMode::eBlockchainMode mode, uint64_t id);
    std::map<uint64_t, std::tuple<std::string, uint64_t, uint64_t>> CStatusBarHub::getCustomStatusBars(eBlockchainMode::eBlockchainMode mode);


    //TODO: refactor!!! this needs to be per DTI and per local terminal! DTI cannot affect local terminal!
    //the currently active cusotm bar ID needs to be PER DTI!!!
    uint64_t getCurrentCustomBarIndex(eBlockchainMode::eBlockchainMode mode);
    void setCurrentCustomBarIndex(eBlockchainMode::eBlockchainMode mode, uint64_t index);
};
