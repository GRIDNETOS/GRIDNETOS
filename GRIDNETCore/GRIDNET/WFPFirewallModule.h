#pragma once

// Target Windows 7 or later for WFP support
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

// Define WIN32_LEAN_AND_MEAN to exclude rarely-used APIs from Windows headers.
// This must come before including windows.h.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// --- BEGIN FIX: Correct Header Order ---

// The correct order is crucial to prevent redefinition errors.
// 1. Include Winsock2 before windows.h to prevent conflicts with the older winsock.h.
#include <winsock2.h>
#include <ws2tcpip.h>

// 2. Include the main windows.h header.
#include <windows.h>

// 3. Include the more specific API headers. These depend on types defined in windows.h.
#include <fwpmu.h>
#include <iphlpapi.h>
#include <ntsecapi.h> // For AuditSetSystemPolicy

// 4. We no longer need winternl.h or the manual NTSTATUS definition,
//    as the headers above provide all necessary types in the correct way.

// --- END FIX ---


// GUID initialization should be included once in the project.
#include <initguid.h>

// Standard library headers
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <queue>
#include <algorithm>
#include "robin_hood.h"
#include <winsvc.h> 

// Link required libraries
#pragma comment(lib, "fwpuclnt.lib")
#pragma comment(lib, "rpcrt4.lib") // Required by fwpmu.h
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")


// Forward declarations
class CNetworkManager;
class CTools;


// GUID definitions for our WFP components
// {A5B4C3D2-1E2F-4A5B-9C8D-7E6F5A4B3C2D}
DEFINE_GUID(GRIDNET_WFP_PROVIDER_GUID,
    0xa5b4c3d2, 0x1e2f, 0x4a5b, 0x9c, 0x8d, 0x7e, 0x6f, 0x5a, 0x4b, 0x3c, 0x2d);

// {B6C5D4E3-2F3A-5B6C-AD9E-8F7A6B5C4D3E}
DEFINE_GUID(GRIDNET_WFP_SUBLAYER_GUID,
    0xb6c5d4e3, 0x2f3a, 0x5b6c, 0xad, 0x9e, 0x8f, 0x7a, 0x6b, 0x5c, 0x4d, 0x3e);

// {C7D6E5F4-3A4B-6C7D-BE0F-9A8B7C6D5E4F}
DEFINE_GUID(GRIDNET_DOS_CALLOUT_GUID,
    0xc7d6e5f4, 0x3a4b, 0x6c7d, 0xbe, 0x0f, 0x9a, 0x8b, 0x7c, 0x6d, 0x5e, 0x4f);

// IP address storage - use numeric format for efficiency
union IPAddress {
    uint32_t ipv4;
    uint8_t ipv6[16];

    bool operator==(const IPAddress& other) const {
        return memcmp(this, &other, sizeof(IPAddress)) == 0;
    }
};

// Hash function for IPAddress
struct IPAddressHash {
    size_t operator()(const IPAddress& ip) const {
        size_t h = 0;
        for (size_t i = 0; i < sizeof(IPAddress); i += sizeof(size_t)) {
            h ^= *reinterpret_cast<const size_t*>(reinterpret_cast<const uint8_t*>(&ip) + i);
        }
        return h;
    }
};

// Enhanced packet info structure
struct PacketInfo {
    IPAddress sourceIP;
    IPAddress destIP;
    uint16_t sourcePort;
    uint16_t destPort;
    uint8_t protocol;
    uint32_t packetSize;
    uint8_t tcpFlags;
    bool isFragmented;
    bool isIPv6;
    std::chrono::steady_clock::time_point timestamp;
};

// DoS detection thresholds with validation
struct DosThresholds {
    uint32_t synFloodThreshold = 100;          // SYN packets per second
    uint32_t udpFloodThreshold = 1000;         // UDP packets per second
    uint32_t icmpFloodThreshold = 50;          // ICMP packets per second
    uint32_t connectionRateThreshold = 50;     // New connections per second
    uint32_t packetRateThreshold = 10000;      // Total packets per second
    uint32_t smallPacketThreshold = 1000;      // Small packets per second
    uint32_t fragmentedPacketThreshold = 100;  // Fragmented packets per second
    uint32_t invalidPacketThreshold = 100;     // Invalid packets per second

    // Validation
    bool validate() const {
        return synFloodThreshold > 0 &&
            udpFloodThreshold > 0 &&
            icmpFloodThreshold > 0 &&
            connectionRateThreshold > 0 &&
            packetRateThreshold > 0;
    }
};

// Attack detection statistics per IP
struct AttackStats {
    std::atomic<uint64_t> synPackets{ 0 };
    std::atomic<uint64_t> udpPackets{ 0 };
    std::atomic<uint64_t> icmpPackets{ 0 };
    std::atomic<uint64_t> totalPackets{ 0 };
    std::atomic<uint64_t> smallPackets{ 0 };
    std::atomic<uint64_t> fragmentedPackets{ 0 };
    std::atomic<uint64_t> invalidPackets{ 0 };
    std::atomic<uint64_t> newConnections{ 0 };
    std::atomic<uint64_t> bytesReceived{ 0 };
    std::chrono::steady_clock::time_point lastReset;
    std::chrono::steady_clock::time_point firstSeen;
    std::chrono::steady_clock::time_point lastSeen;
    std::atomic<bool> isUnderAttack{ false };

    // Rate calculation helper
    double getPacketRate(std::chrono::steady_clock::time_point now) const {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastReset).count();
        return duration > 0 ? static_cast<double>(totalPackets) / duration : 0.0;
    }
};

// Attack types enumeration
enum class AttackType : uint32_t {
    None = 0,
    SynFlood = 1 << 0,
    UdpFlood = 1 << 1,
    IcmpFlood = 1 << 2,
    ConnectionFlood = 1 << 3,
    PacketFlood = 1 << 4,
    SmallPacketFlood = 1 << 5,
    FragmentedPacketFlood = 1 << 6,
    InvalidPacketFlood = 1 << 7,
    SlowlorisAttack = 1 << 8,
    AmplificationAttack = 1 << 9,
    ZeroWindowAttack = 1 << 10,
    LandAttack = 1 << 11
};

// Combine attack types
inline AttackType operator|(AttackType a, AttackType b) {
    return static_cast<AttackType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline AttackType operator&(AttackType a, AttackType b) {
    return static_cast<AttackType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// Blocked IP entry with additional metadata
struct BlockedIPEntry {
    IPAddress ip;
    std::chrono::steady_clock::time_point expireTime;
    AttackType reason;
    uint64_t filterId;
    uint32_t hitCount;
};

// WFP transaction helper class
class WFPTransaction {
public:
    WFPTransaction(HANDLE engineHandle) : mEngineHandle(engineHandle), mCommitted(false) {
        DWORD result = FwpmTransactionBegin(mEngineHandle, 0);
        mSuccess = (result == ERROR_SUCCESS);
    }

    ~WFPTransaction() {
        if (mSuccess && !mCommitted) {
            FwpmTransactionAbort(mEngineHandle);
        }
    }

    bool commit() {
        if (mSuccess && !mCommitted) {
            DWORD result = FwpmTransactionCommit(mEngineHandle);
            mCommitted = (result == ERROR_SUCCESS);
        }
        return mCommitted;
    }

    bool isValid() const { return mSuccess; }

private:
    HANDLE mEngineHandle;
    bool mSuccess;
    bool mCommitted;
};

// WFP Firewall Module class
class CWFPFirewallModule : public std::enable_shared_from_this<CWFPFirewallModule> {
public:
    /*
     * Usage Example:
     *
     * auto firewall = std::make_shared<CWFPFirewallModule>(networkManager);
     *
     * // Initialize the module (requires admin privileges)
     * if (firewall->initialize()) {
     *     // Module is now actively monitoring for attacks
     *
     *     // Optionally configure thresholds
     *     DosThresholds thresholds;
     *     thresholds.synFloodThreshold = 200;  // 200 SYN packets/sec
     *     firewall->setThresholds(thresholds);
     *
     *     // Module runs autonomously - no need to call processIncomingPacket()
     *     // It monitors network activity and blocks attackers automatically
     * }
     *
     * // To manually block an IP:
     * firewall->addBlockedIP("192.168.1.100", 3600, AttackType::SynFlood);
     *
     * // To check statistics:
     * std::vector<std::vector<std::string>> stats;
     * firewall->getAttackStatistics(stats);
     */

    bool setAuditPolicyPrivilege(LPCTSTR privilegeName, BOOL bEnablePrivilege, PTOKEN_PRIVILEGES previousState = nullptr);
    bool programmaticallySetAuditPolicies();

    CWFPFirewallModule(std::shared_ptr<CNetworkManager> networkManager);
    bool restartBfeService();
    ~CWFPFirewallModule();

    // Initialize and shutdown
    bool initialize();
    void shutdown();

    // DoS protection operations
    void processIncomingPacket(const PacketInfo& packetInfo);
    bool shouldBlockPacket(const IPAddress& sourceIP, uint16_t protocol, uint16_t destPort);
    void updateAttackStatistics(const IPAddress& sourceIP, AttackType attackType);

    // IP management
    bool addBlockedIP(const std::string& ip, uint64_t duration, AttackType reason);
    bool removeBlockedIP(const std::string& ip);
    bool isIPBlocked(const IPAddress& ip) const;
    void cleanupExpiredBlocks();

    // Statistics and monitoring
    void getAttackStatistics(std::vector<std::vector<std::string>>& rows);
    void resetStatistics(const std::string& ip = "");
    uint64_t getBlockedIPCount() const;
    std::vector<std::string> getActiveAttacks();

    // Performance metrics
    struct PerformanceMetrics {
        std::atomic<uint64_t> packetsProcessed{ 0 };
        std::atomic<uint64_t> packetsBlocked{ 0 };
        std::atomic<uint64_t> falsePositives{ 0 };
        std::atomic<uint64_t> processingTimeUs{ 0 };
        std::atomic<uint64_t> wfpOperations{ 0 };
        std::atomic<uint64_t> wfpErrors{ 0 };
    };

    //PerformanceMetrics getPerformanceMetrics() const { return mMetrics; }

    // Configuration
    bool setThresholds(const DosThresholds& thresholds);
    DosThresholds getThresholds() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Advanced features
    void enableDeepPacketInspection(bool enable);
    void enableBehavioralAnalysis(bool enable);
    void enableReputationBasedFiltering(bool enable);
    void setWhitelistedPorts(const std::vector<uint16_t>& ports);

    // IP utility functions
    static bool stringToIPAddress(const std::string& ipStr, IPAddress& ip, bool& isIPv6);
    static std::string ipAddressToString(const IPAddress& ip, bool isIPv6);

    void logAttackDetected(const std::string& sourceIP, AttackType attackType);

    void notifyNetworkManager(const std::string& sourceIP, AttackType attackType);

private:
    // WFP management
    bool openWfpSession();
    void closeWfpSession();
    bool registerProvider();
    bool registerSublayer();
    bool registerCallouts();
    bool addFilters();
    void removeFilters();

    // Network monitoring methods
    bool startNetworkMonitoring();
    void stopNetworkMonitoring();
    void networkMonitoringThread();
    bool subscribeToWfpEvents();
    void processWfpEvent(const FWPM_NET_EVENT* netEvent);
    void processConnectionEvent(const FWPM_NET_EVENT* netEvent);
    void gatherNetworkStatistics();
    void analyzeTcpConnections();
    void analyzeUdpEndpoints();

    // Convert WFP net event to packet info
    bool netEventToPacketInfo(const FWPM_NET_EVENT* netEvent, PacketInfo& packetInfo);

    // WFP Callout functions (if needed for future kernel integration)
    // Note: These would require kernel driver implementation
    /*
    static VOID NTAPI classifyFn(
        const FWPS_INCOMING_VALUES* inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
        void* layerData,
        const void* classifyContext,
        const FWPS_FILTER* filter,
        UINT64 flowContext,
        FWPS_CLASSIFY_OUT* classifyOut
    );

    static NTSTATUS NTAPI notifyFn(
        FWPS_CALLOUT_NOTIFY_TYPE notifyType,
        const GUID* filterKey,
        FWPS_FILTER* filter
    );

    static VOID NTAPI flowDeleteFn(
        UINT16 layerId,
        UINT32 calloutId,
        UINT64 flowContext
    );

    // Helper to extract packet info from classify values
    void extractPacketInfo(
        const FWPS_INCOMING_VALUES* inFixedValues,
        UINT16 layerId,
        PacketInfo& packetInfo
    );
    */

    // Attack detection algorithms
    AttackType detectAttackType(const IPAddress& sourceIP, const AttackStats& stats);
    bool detectSynFlood(const AttackStats& stats) const;
    bool detectUdpFlood(const AttackStats& stats) const;
    bool detectConnectionFlood(const AttackStats& stats) const;
    bool detectSlowloris(const std::string& sourceIP);
    bool detectAmplificationAttack(const AttackStats& stats) const;
    bool detectZeroWindowAttack(const AttackStats& stats) const;

    // Packet analysis
    bool isValidPacket(const PacketInfo& packetInfo) const;
    bool isSmallPacket(uint32_t packetSize) const;
    bool isSuspiciousPattern(const PacketInfo& packetInfo) const;

    // Thread-safe operations
    std::shared_ptr<AttackStats> getOrCreateStats(const IPAddress& ip);
    void pruneOldStats();

    // Cleanup thread
    void cleanupThread();

    // Member variables
    std::shared_ptr<CNetworkManager> mNetworkManager;
    std::shared_ptr<CTools> mTools;

    // WFP handles
    HANDLE mEngineHandle;
    UINT32 mCalloutId;
    std::vector<UINT64> mDefaultFilterIds;  // Multiple default filters

    // Configuration
    DosThresholds mThresholds;
    std::atomic<bool> mEnabled;
    std::atomic<bool> mDeepPacketInspection;
    std::atomic<bool> mBehavioralAnalysis;
    std::atomic<bool> mReputationFiltering;
    std::vector<uint16_t> mWhitelistedPorts;
    mutable std::mutex mWhitelistMutex;

    // Attack tracking - improved thread safety
    mutable std::mutex mStatsMutex;
    robin_hood::unordered_map<IPAddress, std::shared_ptr<AttackStats>, IPAddressHash> mAttackStats;

    // Priority queue for efficient oldest entry removal
    struct StatsAge {
        IPAddress ip;
        std::chrono::steady_clock::time_point lastSeen;

        bool operator>(const StatsAge& other) const {
            return lastSeen > other.lastSeen;
        }
    };
    std::priority_queue<StatsAge, std::vector<StatsAge>, std::greater<StatsAge>> mStatsAgeQueue;

    // Blocked IPs - enhanced structure
    mutable std::mutex mBlockedIPsMutex;
    robin_hood::unordered_map<IPAddress, BlockedIPEntry, IPAddressHash> mBlockedIPs;

    // Performance metrics
    PerformanceMetrics mMetrics;

    // Behavioral analysis data
    struct ConnectionBehavior {
        uint32_t avgPacketSize;
        uint32_t avgPacketInterval;
        uint32_t protocolDistribution[256];
        std::chrono::steady_clock::time_point lastUpdate;
    };

    robin_hood::unordered_map<IPAddress, ConnectionBehavior, IPAddressHash> mBehaviorProfiles;
    mutable std::mutex mBehaviorMutex;

    // Cleanup thread management
    std::thread mCleanupThread;
    std::atomic<bool> mShutdownRequested{ false };

    // Network monitoring
    std::thread mMonitoringThread;
    HANDLE mEventHandle;
    HANDLE mSubscriptionHandle;
    std::atomic<bool> mMonitoringActive{ false };

    // Static instance for potential future kernel callbacks
    static CWFPFirewallModule* sInstance;

    // Constants
    static constexpr uint32_t SMALL_PACKET_SIZE = 64;
    static constexpr uint32_t STATS_RESET_INTERVAL_SEC = 60;
    static constexpr uint32_t CLEANUP_INTERVAL_SEC = 30;
    static constexpr uint32_t MAX_TRACKED_IPS = 100000;
    static constexpr uint32_t BEHAVIORAL_LEARNING_PERIOD_SEC = 300;
    static constexpr uint32_t MAX_FILTERS_PER_TRANSACTION = 1000;
    static constexpr uint32_t WFP_TIMEOUT_MS = 30000; // 30 seconds
    static constexpr uint32_t MONITORING_INTERVAL_MS = 100; // 100ms polling
};