#include "WFPFirewallModule.h"
#include "NetworkManager.h"
#include "Tools.h"
#include "CGlobalSecSettings.h"
#include <sstream>

#include <iomanip>
#include <algorithm>
#include <iphlpapi.h>  // For GetTcpStatistics, GetTcpTable, etc.

// In file: WFPFirewallModule.cpp

// Manually define the audit GUIDs because adtschema.h may not be available
// in all Windows SDK installations. This makes the code self-contained.
//
// GUID for "Filtering Platform Packet Drop"
// {0CCE9215-69AE-11D9-BED3-505054503030}
DEFINE_GUID(GUID_AUDIT_FILTERPLATFORMPACKETDROP, 0x0cce9215, 0x69ae, 0x11d9, 0xbe, 0xd3, 0x50, 0x50, 0x54, 0x50, 0x30, 0x30);
//
// GUID for "Filtering Platform Connection"
// {0CCE9216-69AE-11D9-BED3-505054503030}
DEFINE_GUID(GUID_AUDIT_FILTERPLATFORMCONNECTION, 0x0cce9216, 0x69ae, 0x11d9, 0xbe, 0xd3, 0x50, 0x50, 0x54, 0x50, 0x30, 0x30);


// In file: WFPFirewallModule.cpp

/**
 * @brief Enables or disables a specific privilege for the current process token.
 * @param privilegeName The name of the privilege (e.g., SE_SECURITY_NAME).
 * @param bEnablePrivilege TRUE to enable, FALSE to disable.
 * @param previousState A TOKEN_PRIVILEGES structure to receive the previous state, for restoration. Can be nullptr.
 * @return True if the operation was successful.
 */
bool CWFPFirewallModule::setAuditPolicyPrivilege(LPCTSTR privilegeName, BOOL bEnablePrivilege, PTOKEN_PRIVILEGES previousState)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    HANDLE hToken = NULL;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        mTools->logEvent("OpenProcessToken failed.", "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        return false;
    }

    if (!LookupPrivilegeValue(NULL, privilegeName, &luid)) {
        mTools->logEvent("LookupPrivilegeValue failed.", "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

    // --- BEGIN FINAL FIX for ERROR_NOACCESS (998) ---
    // The TOKEN_PRIVILEGES struct is variable-sized. We must provide a buffer large enough
    // for the function to write the previous state into. Simply using sizeof() is not enough
    // and causes a buffer overflow (ERROR_NOACCESS).
    //
    // We create a buffer on the stack that is guaranteed to be large enough for the
    // structure header plus one privilege entry.
    BYTE buffer[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
    PTOKEN_PRIVILEGES pSafePreviousState = (PTOKEN_PRIVILEGES)buffer;
    DWORD dwBufferLength = sizeof(buffer);
    DWORD dwReturnLength = 0;

    // If the caller passed a non-null pointer, we'll use our safe buffer to get the data
    // and then copy it back. If they passed nullptr, we don't need to get the previous state.
    PTOKEN_PRIVILEGES pPreviousStateParam = previousState ? pSafePreviousState : NULL;
    DWORD dwBufferLengthParam = previousState ? dwBufferLength : 0;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, dwBufferLengthParam, pPreviousStateParam, &dwReturnLength)) {
        if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
            mTools->logEvent("The token does not have the specified privilege.", "WFP Firewall", eLogEntryCategory::localSystem, 8, eLogEntryType::warning);
        }
        else {
            mTools->logEvent("AdjustTokenPrivileges failed with error: " + std::to_string(GetLastError()), "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        }
        CloseHandle(hToken);
        return false;
    }

    // If the caller wanted the previous state, copy the result from our safe buffer
    // into their provided struct.
    if (previousState) {
        memcpy(previousState, pSafePreviousState, dwReturnLength);
    }
    // --- END FINAL FIX ---

    CloseHandle(hToken);
    return true;
}

// In file: WFPFirewallModule.cpp

/**
 * @brief Programmatically sets the required WFP audit policies for event collection.
 * This version uses dynamic loading and sets the correct success/failure flags.
 * @return True if the policies were set successfully.
 */
bool CWFPFirewallModule::programmaticallySetAuditPolicies()
{
    // Define a function pointer type for AuditSetSystemPolicy
    typedef BOOLEAN(WINAPI* AuditSetSystemPolicy_t)(PCAUDIT_POLICY_INFORMATION, ULONG);

    HMODULE hAdvApi32 = NULL;
    AuditSetSystemPolicy_t pAuditSetSystemPolicy = NULL;
    bool bSuccess = false;

    hAdvApi32 = LoadLibrary(TEXT("advapi32.dll"));
    if (hAdvApi32 == NULL) {
        mTools->logEvent("Failed to load advapi32.dll.", "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        return false;
    }

    pAuditSetSystemPolicy = (AuditSetSystemPolicy_t)GetProcAddress(hAdvApi32, "AuditSetSystemPolicy");
    if (pAuditSetSystemPolicy == NULL) {
        mTools->logEvent("Failed to get address of AuditSetSystemPolicy.", "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        FreeLibrary(hAdvApi32);
        return false;
    }

    TOKEN_PRIVILEGES previousState;
    memset(&previousState, 0, sizeof(previousState));

    if (!setAuditPolicyPrivilege(SE_SECURITY_NAME, TRUE, &previousState)) {
        mTools->logEvent("Failed to acquire SeSecurityPrivilege to change audit policy.", "WFP Firewall", eLogEntryCategory::localSystem, 8, eLogEntryType::warning);
        FreeLibrary(hAdvApi32);
        return false;
    }

    AUDIT_POLICY_INFORMATION policies[2];
    memset(policies, 0, sizeof(policies));

    // --- BEGIN FINAL FIX ---
    // The WFP engine requires that BOTH success and failure auditing be enabled.
    // We combine the flags with a bitwise OR. This was the missing piece.
    DWORD requiredAuditingFlags = POLICY_AUDIT_EVENT_SUCCESS | POLICY_AUDIT_EVENT_FAILURE;

    // 1. Filtering Platform Packet Drop
    policies[0].AuditSubCategoryGuid = GUID_AUDIT_FILTERPLATFORMPACKETDROP;
    policies[0].AuditingInformation = requiredAuditingFlags;

    // 2. Filtering Platform Connection
    policies[1].AuditSubCategoryGuid = GUID_AUDIT_FILTERPLATFORMCONNECTION;
    policies[1].AuditingInformation = requiredAuditingFlags;
    // --- END FINAL FIX ---

    if (pAuditSetSystemPolicy(policies, 2)) {
        mTools->logEvent("Successfully set system audit policies for WFP event collection.", "WFP Firewall", eLogEntryCategory::localSystem, 5, eLogEntryType::notification);
        bSuccess = true;
    }
    else {
        mTools->logEvent("AuditSetSystemPolicy failed. Error: " + std::to_string(GetLastError()), "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        bSuccess = false;
    }

    // Restore the privilege to its original state
    BOOL wasPreviouslyEnabled = (previousState.PrivilegeCount > 0) && (previousState.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED);
    setAuditPolicyPrivilege(SE_SECURITY_NAME, wasPreviouslyEnabled, nullptr);

    FreeLibrary(hAdvApi32);

    return bSuccess;
}

/*
 * Self-contained WFP DoS Protection Module
 *
 * This module provides DoS protection without requiring kernel drivers by:
 * 1. Monitoring WFP network events (drops, failures)
 * 2. Polling TCP/UDP statistics for anomaly detection
 * 3. Analyzing TCP connection tables for connection floods
 * 4. Creating WFP filters to block attacking IPs
 *
 * The module runs its own monitoring thread that:
 * - Polls WFP events every 100ms
 * - Checks system network statistics
 * - Detects attacks based on patterns
 * - Automatically blocks malicious IPs
 */

 // Static instance initialization
CWFPFirewallModule* CWFPFirewallModule::sInstance = nullptr;


// In file: WFPFirewallModule.cpp
// Add these two new private helper methods to the class implementation.



// Constructor
CWFPFirewallModule::CWFPFirewallModule(std::shared_ptr<CNetworkManager> networkManager)
    : mNetworkManager(networkManager)
    , mEngineHandle(nullptr)
    , mCalloutId(0)
    , mEnabled(true)
    , mDeepPacketInspection(true)
    , mBehavioralAnalysis(true)
    , mReputationFiltering(true)
    , mShutdownRequested(false)
    , mEventHandle(nullptr)
    , mSubscriptionHandle(nullptr)
    , mMonitoringActive(false) {

    mTools = networkManager->getTools();
    sInstance = this;  // Set static instance for potential callbacks
}

 // In file: WFPFirewallModule.cpp

 /**
  * @brief Restarts the Base Filtering Engine (BFE) service to force it to re-read audit policies.
  * @return True if the service was successfully restarted.
  */
bool CWFPFirewallModule::restartBfeService()
{
    mTools->logEvent("Attempting to restart Base Filtering Engine (BFE) service...", "WFP Firewall", eLogEntryCategory::localSystem, 5, eLogEntryType::notification);

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        mTools->logEvent("Failed to open Service Control Manager. Error: " + std::to_string(GetLastError()), "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        return false;
    }

    SC_HANDLE hBfeService = OpenService(hSCManager, L"BFE", SERVICE_ALL_ACCESS);
    if (hBfeService == NULL) {
        mTools->logEvent("Failed to open BFE service handle. Error: " + std::to_string(GetLastError()), "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
        CloseServiceHandle(hSCManager);
        return false;
    }

    SERVICE_STATUS_PROCESS ssp;
    DWORD dwBytesNeeded;

    // --- BEGIN FINAL FIX: Use the correct identifier SC_STATUS_PROCESS_INFO ---
    // Stop the service if it is running.
    if (QueryServiceStatusEx(hBfeService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &dwBytesNeeded)) {
        if (ssp.dwCurrentState == SERVICE_RUNNING || ssp.dwCurrentState == SERVICE_START_PENDING) {
            if (!ControlService(hBfeService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) {
                if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
                    mTools->logEvent("Failed to send stop control to BFE service. Error: " + std::to_string(GetLastError()), "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
                }
            }

            // Wait for the service to actually stop.
            for (int i = 0; i < 30; ++i) { // 30 second timeout
                Sleep(1000);
                if (!QueryServiceStatusEx(hBfeService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &dwBytesNeeded)) break;
                if (ssp.dwCurrentState == SERVICE_STOPPED) break;
            }

            if (ssp.dwCurrentState != SERVICE_STOPPED) {
                mTools->logEvent("Timeout waiting for BFE service to stop.", "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
                CloseServiceHandle(hBfeService);
                CloseServiceHandle(hSCManager);
                return false;
            }
        }
    }

    // Start the service.
    if (!StartService(hBfeService, 0, NULL)) {
        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
            mTools->logEvent("Failed to start BFE service. Error: " + std::to_string(GetLastError()), "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
            CloseServiceHandle(hBfeService);
            CloseServiceHandle(hSCManager);
            return false;
        }
    }

    // Wait for the service to be in the running state.
    for (int i = 0; i < 30; ++i) { // 30 second timeout
        Sleep(1000);
        if (!QueryServiceStatusEx(hBfeService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &dwBytesNeeded)) break;
        if (ssp.dwCurrentState == SERVICE_RUNNING) break;
    }
    // --- END FINAL FIX ---

    CloseServiceHandle(hBfeService);
    CloseServiceHandle(hSCManager);

    if (ssp.dwCurrentState == SERVICE_RUNNING) {
        mTools->logEvent("BFE service successfully restarted.", "WFP Firewall", eLogEntryCategory::localSystem, 5, eLogEntryType::notification);
        return true;
    }

    mTools->logEvent("BFE service did not enter running state after restart attempt.", "WFP Firewall", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
    return false;
}

// Destructor
CWFPFirewallModule::~CWFPFirewallModule() {
    // Clear static instance
    if (sInstance == this) {
        sInstance = nullptr;
    }

    // Signal shutdown
    mShutdownRequested = true;
    mEnabled = false;
    mMonitoringActive = false;

    // Stop monitoring
    if (mEventHandle) {
        SetEvent(mEventHandle);
    }

    // Wait for threads
    if (mMonitoringThread.joinable()) {
        mMonitoringThread.join();
    }

    if (mCleanupThread.joinable()) {
        mCleanupThread.join();
    }

    // Shutdown if still initialized
    if (mEngineHandle != nullptr) {
        shutdown();
    }
}

// Initialize the WFP module
// In file WFPFirewallModule.cpp

// Initialize the WFP module
// In file: WFPFirewallModule.cpp

// Initialize the WFP module
// In file WFPFirewallModule.cpp

// Initialize the WFP module
// Initialize the WFP module
// In file: WFPFirewallModule.cpp

// Initialize the WFP module
// In file: WFPFirewallModule.cpp

/**
 * @brief Initializes the WFP Firewall module, including programmatic self-configuration of system policies.
 */
bool CWFPFirewallModule::initialize()
{
    mTools->logEvent("Initializing Windows Filtering Platform DoS Protection Module...", "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification);

    if (!mTools->isRunningAsAdmin() && !mTools->isElevatedAndInNetworkConfigurationOperatorsGroup()) {
        mTools->logEvent("WFP module requires administrator privileges. Module disabled.", "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
        return false;
    }

    bool wfpEventingAvailable = false;
    FWP_VALUE0 value;
    memset(&value, 0, sizeof(value));
    value.type = FWP_UINT32;
    value.uint32 = 1; // 1 to enable

    // STEP 1: Attempt to enable WFP event collection.
    DWORD result = FwpmEngineSetOption(nullptr, FWPM_ENGINE_COLLECT_NET_EVENTS, &value);

    if (result == ERROR_SUCCESS) {
        wfpEventingAvailable = true;
        mTools->logEvent("WFP real-time event collection is already enabled.", "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);
    }
    else if (result == FWP_E_NET_EVENTS_DISABLED) {
        // This is the main self-remediation path.
        mTools->logEvent("WFP eventing disabled by system policy. Attempting to enable programmatically...", "WFP Firewall", eLogEntryCategory::network, 7, eLogEntryType::notification);

        // STEP 1a: Set the required audit policies.
        if (programmaticallySetAuditPolicies()) {

            // STEP 1b: Restart the BFE service to force it to re-read the new policy.
            if (restartBfeService()) {
                // Give the service a moment to stabilize after restart.
                Sleep(2000);

                // STEP 1c: Retry enabling event collection. This should now succeed.
                result = FwpmEngineSetOption(nullptr, FWPM_ENGINE_COLLECT_NET_EVENTS, &value);
                if (result == ERROR_SUCCESS) {
                    wfpEventingAvailable = true;
                    mTools->logEvent("Successfully enabled WFP real-time event collection after remediation.", "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification, eColor::lightGreen);
                }
                else {
                    mTools->logEvent("Failed to enable WFP eventing even after setting policy and restarting BFE. Falling back to polling.", "WFP Firewall", eLogEntryCategory::network, 8, eLogEntryType::warning);
                }
            }
            else {
                mTools->logEvent("Failed to restart BFE service. Policy change may not apply. Falling back to polling.", "WFP Firewall", eLogEntryCategory::network, 8, eLogEntryType::warning);
            }
        }
        else {
            mTools->logEvent("Failed to programmatically set audit policy. Falling back to polling.", "WFP Firewall", eLogEntryCategory::network, 8, eLogEntryType::warning);
        }
    }
    else {
        // Any other error is unexpected and should be treated as a failure.
        std::stringstream ss;
        ss << "Failed to set global WFP engine options. Error: 0x" << std::hex << result;
        mTools->logEvent(ss.str(), "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
        return false;
    }

    // STEP 2: Open our client session and add filters. This is required regardless of eventing status.
    if (!openWfpSession()) {
        mTools->logEvent("Failed to open WFP session.", "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
        return false;
    }

    {
        WFPTransaction transaction(mEngineHandle);
        if (!transaction.isValid() || !registerProvider() || !registerSublayer() || !addFilters() || !transaction.commit()) {
            mTools->logEvent("Failed during WFP object registration transaction.", "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
            closeWfpSession();
            return false;
        }
    }

    // STEP 3: Start background threads.
    mCleanupThread = std::thread(&CWFPFirewallModule::cleanupThread, this);

    // The monitoring thread will use events if available, otherwise it will just poll.
    mMonitoringActive = true;
    if (wfpEventingAvailable) {
        if (!startNetworkMonitoring()) {
            mTools->logEvent("Subscription to WFP events failed. Monitoring will be polling-only.", "WFP Firewall", eLogEntryCategory::network, 8, eLogEntryType::warning);
            // Fall through to start the thread for polling anyway
            if (!mMonitoringThread.joinable()) {
                mMonitoringThread = std::thread(&CWFPFirewallModule::networkMonitoringThread, this);
            }
        }
    }
    else {
        // Start the thread for polling-only mode.
        mMonitoringThread = std::thread(&CWFPFirewallModule::networkMonitoringThread, this);
    }

    mTools->logEvent("WFP DoS Protection Module initialized successfully.", "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification, eColor::lightGreen);
    return true;
}

void CWFPFirewallModule::shutdown() {
    if (mEngineHandle) {
        mTools->logEvent("Shutting down WFP DoS Protection Module...",
            "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification);

        // Signal shutdown
        mShutdownRequested = true;

        // Stop monitoring
        stopNetworkMonitoring();

        // Remove filters using transaction
        {
            WFPTransaction transaction(mEngineHandle);
            if (transaction.isValid()) {
                removeFilters();
                transaction.commit();
            }
        }

        closeWfpSession();

        mTools->logEvent("WFP DoS Protection Module shut down",
            "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification);
    }
}

// Open WFP session
bool CWFPFirewallModule::openWfpSession() {
    FWPM_SESSION session = { 0 };
    session.displayData.name = L"GRIDNET Core WFP Session";
    session.displayData.description = L"Windows Filtering Platform session for GRIDNET Core DoS protection";

    // --- BEGIN FIX ---
    // DO NOT use a dynamic session. Setting global engine options like FWPM_ENGINE_COLLECT_NET_EVENTS
    // is not permitted from a dynamic session. A standard session is required.
    // Your existing shutdown() logic already handles manual cleanup, so this is safe.
    session.flags = 0; // REPLACED: session.flags = FWPM_SESSION_FLAG_DYNAMIC;
    // --- END FIX ---

    session.txnWaitTimeoutInMSec = WFP_TIMEOUT_MS;

    DWORD result = FwpmEngineOpen(
        nullptr,
        RPC_C_AUTHN_WINNT,
        nullptr,
        &session,
        &mEngineHandle
    );

    if (result != ERROR_SUCCESS) {
        mTools->logEvent("Failed to open WFP engine: 0x" +
            std::to_string(result) + " (" + std::to_string(result) + ")",
            "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
        return false;
    }

    return true;
}
// Close WFP session
void CWFPFirewallModule::closeWfpSession() {
    if (mEngineHandle) {
        FwpmEngineClose(mEngineHandle);
        mEngineHandle = nullptr;
    }
}

// Register WFP provider
bool CWFPFirewallModule::registerProvider() {
    FWPM_PROVIDER provider = { 0 };
    provider.providerKey = GRIDNET_WFP_PROVIDER_GUID;
    provider.displayData.name = L"GRIDNET Core WFP Provider";
    provider.displayData.description = L"Provider for GRIDNET Core DoS protection";

    DWORD result = FwpmProviderAdd(mEngineHandle, &provider, nullptr);

    if (result != ERROR_SUCCESS && result != FWP_E_ALREADY_EXISTS) {
        mTools->logEvent("Failed to register WFP provider: 0x" +
            std::to_string(result),
            "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
        return false;
    }

    return true;
}

// Register WFP sublayer
bool CWFPFirewallModule::registerSublayer() {
    FWPM_SUBLAYER sublayer = { 0 };
    sublayer.subLayerKey = GRIDNET_WFP_SUBLAYER_GUID;
    sublayer.displayData.name = L"GRIDNET Core DoS Protection Sublayer";
    sublayer.displayData.description = L"Sublayer for GRIDNET Core DoS protection filters";
    sublayer.providerKey = const_cast<GUID*>(&GRIDNET_WFP_PROVIDER_GUID);
    sublayer.weight = 65535; // High priority

    DWORD result = FwpmSubLayerAdd(mEngineHandle, &sublayer, nullptr);

    if (result != ERROR_SUCCESS && result != FWP_E_ALREADY_EXISTS) {
        mTools->logEvent("Failed to register WFP sublayer: 0x" +
            std::to_string(result),
            "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
        return false;
    }

    return true;
}

// Register callouts
bool CWFPFirewallModule::registerCallouts() {
    // In a full implementation, this would register kernel-mode callouts
    // For this integration, we'll use the existing packet processing
    return true;
}

// Add WFP filters
bool CWFPFirewallModule::addFilters() {
    // Add filters for both IPv4 and IPv6
    struct FilterConfig {
        const wchar_t* name;
        GUID layerKey;
        bool isIPv6;
    };

    FilterConfig configs[] = {
        { L"GRIDNET Core DoS Protection Filter IPv4", FWPM_LAYER_INBOUND_IPPACKET_V4, false },
        { L"GRIDNET Core DoS Protection Filter IPv6", FWPM_LAYER_INBOUND_IPPACKET_V6, true }
    };

    for (const auto& config : configs) {
        FWPM_FILTER filter = { 0 };
        UINT64 weightValue = 100;

        filter.displayData.name = const_cast<wchar_t*>(config.name);
        filter.displayData.description = L"Permits traffic by default, blocks are added dynamically";
        filter.providerKey = const_cast<GUID*>(&GRIDNET_WFP_PROVIDER_GUID);
        filter.layerKey = config.layerKey;
        filter.subLayerKey = GRIDNET_WFP_SUBLAYER_GUID;
        filter.weight.type = FWP_UINT64;
        filter.weight.uint64 = &weightValue;
        filter.action.type = FWP_ACTION_PERMIT;

        UINT64 filterId = 0;
        DWORD result = FwpmFilterAdd(mEngineHandle, &filter, nullptr, &filterId);

        if (result != ERROR_SUCCESS) {
            mTools->logEvent("Failed to add WFP filter: 0x" +
                std::to_string(result),
                "WFP Firewall", eLogEntryCategory::network, 10, eLogEntryType::failure);
            return false;
        }

        mDefaultFilterIds.push_back(filterId);
    }

    return true;
}

// Remove WFP filters
void CWFPFirewallModule::removeFilters() {
    // Remove default filters
    for (UINT64 filterId : mDefaultFilterIds) {
        if (mEngineHandle != nullptr) {
            FwpmFilterDeleteById(mEngineHandle, filterId);
        }
    }
    mDefaultFilterIds.clear();

    // Remove all blocked IP filters
    std::vector<UINT64> filterIds;
    {
        std::lock_guard<std::mutex> lock(mBlockedIPsMutex);
        for (const auto& [ip, entry] : mBlockedIPs) {
            if (entry.filterId != 0) {
                filterIds.push_back(entry.filterId);
            }
        }
        mBlockedIPs.clear();
    }

    // Delete filters in batches
    const size_t batchSize = 100;
    for (size_t i = 0; i < filterIds.size(); i += batchSize) {
        WFPTransaction transaction(mEngineHandle);
        if (transaction.isValid()) {
            size_t end = min(i + batchSize, filterIds.size());
            for (size_t j = i; j < end; ++j) {
                FwpmFilterDeleteById(mEngineHandle, filterIds[j]);
            }
            transaction.commit();
        }
    }

    mTools->logEvent("Removed all WFP filters",
        "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);
}

// Process incoming packet
void CWFPFirewallModule::processIncomingPacket(const PacketInfo& packetInfo) {
    // This method is called internally by:
    // 1. networkMonitoringThread() when WFP events are detected
    // 2. gatherNetworkStatistics() when suspicious patterns are found
    // 3. analyzeTcpConnections() when connection floods are detected
    // The module is completely self-contained and autonomous.

    if (!mEnabled) {
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    mMetrics.packetsProcessed++;

    // Check if IP is already blocked
    if (isIPBlocked(packetInfo.sourceIP)) {
        mMetrics.packetsBlocked++;
        return;
    }

    // Get or create statistics
    auto stats = getOrCreateStats(packetInfo.sourceIP);
    if (!stats) {
        return; // Memory allocation failed
    }

    // Update statistics atomically
    stats->totalPackets++;
    stats->bytesReceived += packetInfo.packetSize;
    stats->lastSeen = std::chrono::steady_clock::now();

    switch (packetInfo.protocol) {
    case IPPROTO_TCP:
        // Check for SYN flood
        if (packetInfo.tcpFlags & 0x02 && !(packetInfo.tcpFlags & 0x10)) { // SYN without ACK
            stats->synPackets++;
        }
        break;
    case IPPROTO_UDP:
        stats->udpPackets++;
        break;
    case IPPROTO_ICMP:
        stats->icmpPackets++;
        break;
    }

    // Check packet characteristics
    if (isSmallPacket(packetInfo.packetSize)) {
        stats->smallPackets++;
    }

    if (packetInfo.isFragmented) {
        stats->fragmentedPackets++;
    }

    if (!isValidPacket(packetInfo)) {
        stats->invalidPackets++;
    }

    // Check if we need to evaluate for attacks
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - stats->lastReset).count();

    if (duration >= STATS_RESET_INTERVAL_SEC) {
        // Evaluate attack status
        AttackType attackType = detectAttackType(packetInfo.sourceIP, *stats);

        if (attackType != AttackType::None) {
            // Attack detected
            mMetrics.packetsBlocked++;
            stats->isUnderAttack = true;

            logAttackDetected(ipAddressToString(packetInfo.sourceIP, packetInfo.isIPv6), attackType);

            // Block the IP
            std::string ipStr = ipAddressToString(packetInfo.sourceIP, packetInfo.isIPv6);
            addBlockedIP(ipStr, CGlobalSecSettings::getBanDTIPeersForSecs(), attackType);

            notifyNetworkManager(ipStr, attackType);
        }
        else {
            stats->isUnderAttack = false;
        }

        // Reset counters for next interval
        stats->synPackets = 0;
        stats->udpPackets = 0;
        stats->icmpPackets = 0;
        stats->totalPackets = 0;
        stats->smallPackets = 0;
        stats->fragmentedPackets = 0;
        stats->invalidPackets = 0;
        stats->bytesReceived = 0;
        stats->newConnections = 0;
        stats->lastReset = now;
    }

    // Update performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    mMetrics.processingTimeUs += processingTime;
}

// Get or create stats with improved thread safety
std::shared_ptr<AttackStats> CWFPFirewallModule::getOrCreateStats(const IPAddress& ip) {
    std::lock_guard<std::mutex> lock(mStatsMutex);

    // Check if we need to prune old entries
    if (mAttackStats.size() >= MAX_TRACKED_IPS) {
        pruneOldStats();
    }

    // Get or create stats
    auto& statsPtr = mAttackStats[ip];
    if (!statsPtr) {
        try {
            statsPtr = std::make_shared<AttackStats>();
            statsPtr->lastReset = std::chrono::steady_clock::now();
            statsPtr->firstSeen = statsPtr->lastReset;
            statsPtr->lastSeen = statsPtr->lastReset;
        }
        catch (const std::bad_alloc&) {
            mTools->logEvent("Failed to allocate memory for attack statistics",
                "WFP Firewall", eLogEntryCategory::network, 8, eLogEntryType::warning);
            return nullptr;
        }
    }

    return statsPtr;
}

// Prune old statistics entries
void CWFPFirewallModule::pruneOldStats() {
    // Already under mStatsMutex lock
    auto now = std::chrono::steady_clock::now();
    const auto maxAge = std::chrono::minutes(30);

    // Find entries to remove
    std::vector<IPAddress> toRemove;
    for (const auto& [ip, stats] : mAttackStats) {
        if (stats && !stats->isUnderAttack &&
            (now - stats->lastSeen) > maxAge) {
            toRemove.push_back(ip);
        }
    }

    // Remove old entries
    for (const auto& ip : toRemove) {
        mAttackStats.erase(ip);
    }

    if (!toRemove.empty()) {
        mTools->logEvent("Pruned " + std::to_string(toRemove.size()) + " old attack statistics entries",
            "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);
    }
}

// Detect attack type based on statistics
AttackType CWFPFirewallModule::detectAttackType(const IPAddress& sourceIP, const AttackStats& stats) {
    AttackType attackType = AttackType::None;

    if (detectSynFlood(stats)) {
        attackType = attackType | AttackType::SynFlood;
    }

    if (detectUdpFlood(stats)) {
        attackType = attackType | AttackType::UdpFlood;
    }

    if (stats.icmpPackets > mThresholds.icmpFloodThreshold) {
        attackType = attackType | AttackType::IcmpFlood;
    }

    if (detectConnectionFlood(stats)) {
        attackType = attackType | AttackType::ConnectionFlood;
    }

    auto rate = stats.getPacketRate(std::chrono::steady_clock::now());
    if (rate > mThresholds.packetRateThreshold) {
        attackType = attackType | AttackType::PacketFlood;
    }

    if (stats.smallPackets > mThresholds.smallPacketThreshold) {
        attackType = attackType | AttackType::SmallPacketFlood;
    }

    if (stats.fragmentedPackets > mThresholds.fragmentedPacketThreshold) {
        attackType = attackType | AttackType::FragmentedPacketFlood;
    }

    if (stats.invalidPackets > mThresholds.invalidPacketThreshold) {
        attackType = attackType | AttackType::InvalidPacketFlood;
    }

    // Additional attack detection
    if (detectAmplificationAttack(stats)) {
        attackType = attackType | AttackType::AmplificationAttack;
    }

    if (detectZeroWindowAttack(stats)) {
        attackType = attackType | AttackType::ZeroWindowAttack;
    }

    return attackType;
}

// Detect SYN flood
bool CWFPFirewallModule::detectSynFlood(const AttackStats& stats) const {
    return stats.synPackets > mThresholds.synFloodThreshold;
}

// Detect UDP flood
bool CWFPFirewallModule::detectUdpFlood(const AttackStats& stats) const {
    return stats.udpPackets > mThresholds.udpFloodThreshold;
}

// Detect connection flood
bool CWFPFirewallModule::detectConnectionFlood(const AttackStats& stats) const {
    return stats.newConnections > mThresholds.connectionRateThreshold;
}

// Detect Slowloris attack
bool CWFPFirewallModule::detectSlowloris(const std::string& sourceIP) {
    // Check for patterns indicative of Slowloris
    auto connCount = mNetworkManager->getConnectionAttemptsFrom(
        const_cast<std::string&>(sourceIP),
        eTransportType::HTTPConnection,
        300 // 5 minutes
    );

    return connCount > 20; // Threshold for suspected Slowloris
}

// Detect amplification attack
bool CWFPFirewallModule::detectAmplificationAttack(const AttackStats& stats) const {
    // More sophisticated detection:
    // 1. High ratio of small packets
    // 2. Specific port patterns (DNS:53, NTP:123, SSDP:1900)
    // 3. Asymmetric traffic patterns

    if (stats.totalPackets == 0) return false;

    double smallPacketRatio = static_cast<double>(stats.smallPackets) / stats.totalPackets;
    double avgPacketSize = stats.bytesReceived / stats.totalPackets;

    return smallPacketRatio > 0.8 && avgPacketSize < 100;
}

// Detect zero window attack
bool CWFPFirewallModule::detectZeroWindowAttack(const AttackStats& stats) const {
    // This would require deeper packet inspection
    // For now, return false
    return false;
}

// Check if packet should be blocked
bool CWFPFirewallModule::shouldBlockPacket(const IPAddress& sourceIP, uint16_t protocol, uint16_t destPort) {
    if (!mEnabled) {
        return false;
    }

    // Check if IP is blocked
    if (isIPBlocked(sourceIP)) {
        return true;
    }

    // Check if IP is under active attack
    {
        std::lock_guard<std::mutex> lock(mStatsMutex);
        auto it = mAttackStats.find(sourceIP);
        if (it != mAttackStats.end() && it->second && it->second->isUnderAttack) {
            return true;
        }
    }

    // Check whitelisted ports
    {
        std::lock_guard<std::mutex> lock(mWhitelistMutex);
        if (!mWhitelistedPorts.empty()) {
            bool isWhitelisted = std::find(mWhitelistedPorts.begin(),
                mWhitelistedPorts.end(),
                destPort) != mWhitelistedPorts.end();
            if (!isWhitelisted) {
                // Log but don't block non-whitelisted ports for now
                mTools->logEvent("Non-whitelisted port access: " + std::to_string(destPort),
                    "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);
            }
        }
    }

    return false;
}

// Check if packet is valid
bool CWFPFirewallModule::isValidPacket(const PacketInfo& packetInfo) const {
    // Basic validation checks

    // Check for invalid packet sizes
    if (packetInfo.packetSize < 20) { // Minimum IP header size
        return false;
    }

    // Check for invalid port numbers
    if (packetInfo.protocol == IPPROTO_TCP || packetInfo.protocol == IPPROTO_UDP) {
        if (packetInfo.sourcePort == 0 || packetInfo.destPort == 0) {
            return false;
        }
    }

    // Check for suspicious patterns
    if (isSuspiciousPattern(packetInfo)) {
        return false;
    }

    return true;
}

// Check for suspicious patterns
bool CWFPFirewallModule::isSuspiciousPattern(const PacketInfo& packetInfo) const {
    // Land attack - source IP equals destination IP
    if (memcmp(&packetInfo.sourceIP, &packetInfo.destIP, sizeof(IPAddress)) == 0) {
        return true;
    }

    // Christmas tree packet (all TCP flags set)
    if (packetInfo.protocol == IPPROTO_TCP && packetInfo.tcpFlags == 0xFF) {
        return true;
    }

    // Null scan (no TCP flags)
    if (packetInfo.protocol == IPPROTO_TCP && packetInfo.tcpFlags == 0) {
        return true;
    }

    return false;
}

// Check if packet is small
bool CWFPFirewallModule::isSmallPacket(uint32_t packetSize) const {
    return packetSize <= SMALL_PACKET_SIZE;
}

// Add blocked IP
bool CWFPFirewallModule::addBlockedIP(const std::string& ipStr, uint64_t duration, AttackType reason) {
    IPAddress ip;
    bool isIPv6;

    if (!stringToIPAddress(ipStr, ip, isIPv6)) {
        mTools->logEvent("Invalid IP address: " + ipStr,
            "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::warning);
        return false;
    }

    auto expireTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

    // Create block entry
    BlockedIPEntry entry;
    entry.ip = ip;
    entry.expireTime = expireTime;
    entry.reason = reason;
    entry.filterId = 0;
    entry.hitCount = 0;

    // Create WFP filter
    UINT64 filterId = 0;
    {
        WFPTransaction transaction(mEngineHandle);
        if (!transaction.isValid()) {
            mMetrics.wfpErrors++;
            return false;
        }

        FWPM_FILTER blockFilter = { 0 };
        UINT64 weightValue = 1000;

        // Create name and description
        std::wstring filterName = L"GRIDNET Block " + std::wstring(ipStr.begin(), ipStr.end());
        std::wstring filterDesc = L"Blocking IP for DoS attack";

        blockFilter.displayData.name = const_cast<wchar_t*>(filterName.c_str());
        blockFilter.displayData.description = const_cast<wchar_t*>(filterDesc.c_str());
        blockFilter.providerKey = const_cast<GUID*>(&GRIDNET_WFP_PROVIDER_GUID);
        blockFilter.layerKey = isIPv6 ? FWPM_LAYER_INBOUND_IPPACKET_V6 : FWPM_LAYER_INBOUND_IPPACKET_V4;
        blockFilter.subLayerKey = GRIDNET_WFP_SUBLAYER_GUID;
        blockFilter.weight.type = FWP_UINT64;
        blockFilter.weight.uint64 = &weightValue;
        blockFilter.action.type = FWP_ACTION_BLOCK;

        // Set condition for source IP
        FWPM_FILTER_CONDITION condition = { 0 };
        condition.fieldKey = isIPv6 ? FWPM_CONDITION_IP_SOURCE_ADDRESS : FWPM_CONDITION_IP_SOURCE_ADDRESS;
        condition.matchType = FWP_MATCH_EQUAL;

        if (isIPv6) {
            condition.conditionValue.type = FWP_BYTE_ARRAY16_TYPE;
            condition.conditionValue.byteArray16 = (FWP_BYTE_ARRAY16*)&ip.ipv6;
        }
        else {
            condition.conditionValue.type = FWP_UINT32;
            condition.conditionValue.uint32 = ip.ipv4;
        }

        blockFilter.filterCondition = &condition;
        blockFilter.numFilterConditions = 1;

        DWORD result = FwpmFilterAdd(mEngineHandle, &blockFilter, nullptr, &filterId);

        if (result != ERROR_SUCCESS) {
            mTools->logEvent("Failed to add WFP block filter for IP " + ipStr + ": 0x" +
                std::to_string(result),
                "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::failure);
            mMetrics.wfpErrors++;
            return false;
        }

        if (!transaction.commit()) {
            mMetrics.wfpErrors++;
            return false;
        }

        mMetrics.wfpOperations++;
    }

    // Store filter ID
    entry.filterId = filterId;

    // Add to blocked list
    {
        std::lock_guard<std::mutex> lock(mBlockedIPsMutex);
        mBlockedIPs[ip] = entry;
    }

    // Notify network manager
    mNetworkManager->banIP(ipStr, duration, false);

    // Log the action
    std::stringstream reasonStr;
    if (static_cast<uint32_t>(reason) & static_cast<uint32_t>(AttackType::SynFlood)) reasonStr << "SYN-Flood ";
    if (static_cast<uint32_t>(reason) & static_cast<uint32_t>(AttackType::UdpFlood)) reasonStr << "UDP-Flood ";
    if (static_cast<uint32_t>(reason) & static_cast<uint32_t>(AttackType::IcmpFlood)) reasonStr << "ICMP-Flood ";

    mTools->logEvent("Blocked IP " + ipStr + " for " + std::to_string(duration) +
        " seconds. Reason: " + reasonStr.str() + "(Filter ID: " + std::to_string(filterId) + ")",
        "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification, eColor::lightPink);

    return true;
}

// Remove blocked IP
bool CWFPFirewallModule::removeBlockedIP(const std::string& ipStr) {
    IPAddress ip;
    bool isIPv6;

    if (!stringToIPAddress(ipStr, ip, isIPv6)) {
        return false;
    }

    UINT64 filterId = 0;

    {
        std::lock_guard<std::mutex> lock(mBlockedIPsMutex);

        auto it = mBlockedIPs.find(ip);
        if (it != mBlockedIPs.end()) {
            filterId = it->second.filterId;
            mBlockedIPs.erase(it);
        }
        else {
            return false; // Not found
        }
    }

    // Remove WFP filter
    if (filterId != 0 && mEngineHandle != nullptr) {
        WFPTransaction transaction(mEngineHandle);
        if (transaction.isValid()) {
            DWORD result = FwpmFilterDeleteById(mEngineHandle, filterId);
            if (result == ERROR_SUCCESS) {
                transaction.commit();
                mMetrics.wfpOperations++;
            }
            else {
                mTools->logEvent("Failed to remove WFP filter for IP " + ipStr +
                    ": 0x" + std::to_string(result),
                    "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::warning);
                mMetrics.wfpErrors++;
            }
        }
    }

    // Remove from network manager
    std::vector<uint8_t> ipBytes = mTools->stringToBytes(ipStr);
    mNetworkManager->pardonPeer(ipBytes);

    mTools->logEvent("Removed IP block for " + ipStr,
        "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);

    return true;
}

// Check if IP is blocked
bool CWFPFirewallModule::isIPBlocked(const IPAddress& ip) const {
    std::lock_guard<std::mutex> lock(mBlockedIPsMutex);

    auto it = mBlockedIPs.find(ip);
    if (it != mBlockedIPs.end()) {
        return std::chrono::steady_clock::now() < it->second.expireTime;
    }

    return false;
}

// Cleanup expired blocks
void CWFPFirewallModule::cleanupExpiredBlocks() {
    std::vector<std::pair<IPAddress, UINT64>> toRemove;
    auto now = std::chrono::steady_clock::now();

    // Find expired entries
    {
        std::lock_guard<std::mutex> lock(mBlockedIPsMutex);

        for (const auto& [ip, entry] : mBlockedIPs) {
            if (now >= entry.expireTime) {
                toRemove.push_back({ ip, entry.filterId });
            }
        }

        // Remove from map
        for (const auto& [ip, filterId] : toRemove) {
            mBlockedIPs.erase(ip);
        }
    }

    // Remove WFP filters
    if (!toRemove.empty() && mEngineHandle != nullptr) {
        WFPTransaction transaction(mEngineHandle);
        if (transaction.isValid()) {
            for (const auto& [ip, filterId] : toRemove) {
                if (filterId != 0) {
                    FwpmFilterDeleteById(mEngineHandle, filterId);
                }
            }
            transaction.commit();
            mMetrics.wfpOperations++;
        }
    }

    // Notify network manager
    for (const auto& [ip, filterId] : toRemove) {
        std::string ipStr = ipAddressToString(ip, false); // TODO: track IPv6
        std::vector<uint8_t> ipBytes = mTools->stringToBytes(ipStr);
        mNetworkManager->pardonPeer(ipBytes);
    }

    if (!toRemove.empty()) {
        mTools->logEvent("Cleaned up " + std::to_string(toRemove.size()) + " expired IP blocks",
            "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);
    }
}

// Cleanup thread
void CWFPFirewallModule::cleanupThread() {
    while (!mShutdownRequested) {
        // Sleep for cleanup interval
        for (int i = 0; i < CLEANUP_INTERVAL_SEC && !mShutdownRequested; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (mShutdownRequested) break;

        // Perform cleanup
        try {
            cleanupExpiredBlocks();

            // Also prune old statistics periodically
            {
                std::lock_guard<std::mutex> lock(mStatsMutex);
                pruneOldStats();
            }
        }
        catch (const std::exception& e) {
            mTools->logEvent("Exception in cleanup thread: " + std::string(e.what()),
                "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::warning);
        }
    }
}

// Get attack statistics
void CWFPFirewallModule::getAttackStatistics(std::vector<std::vector<std::string>>& rows) {
    rows.clear();

    // Header row
    rows.push_back({
        "IP Address", "Total Packets", "Packet Rate", "SYN Packets",
        "UDP Packets", "ICMP Packets", "Small Packets", "Invalid Packets",
        "Bytes Received", "Status"
        });

    std::lock_guard<std::mutex> lock(mStatsMutex);
    auto now = std::chrono::steady_clock::now();

    for (const auto& [ip, stats] : mAttackStats) {
        if (!stats) continue;

        std::vector<std::string> row;

        // Convert IP to string (assuming IPv4 for display)
        row.push_back(ipAddressToString(ip, false));
        row.push_back(std::to_string(stats->totalPackets.load()));

        // Calculate packet rate
        double rate = stats->getPacketRate(now);
        std::stringstream rateStr;
        rateStr << std::fixed << std::setprecision(1) << rate << " pps";
        row.push_back(rateStr.str());

        row.push_back(std::to_string(stats->synPackets.load()));
        row.push_back(std::to_string(stats->udpPackets.load()));
        row.push_back(std::to_string(stats->icmpPackets.load()));
        row.push_back(std::to_string(stats->smallPackets.load()));
        row.push_back(std::to_string(stats->invalidPackets.load()));

        // Format bytes
        uint64_t bytes = stats->bytesReceived.load();
        std::stringstream bytesStr;
        if (bytes > 1024 * 1024) {
            bytesStr << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        }
        else if (bytes > 1024) {
            bytesStr << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        }
        else {
            bytesStr << bytes << " B";
        }
        row.push_back(bytesStr.str());

        // Status
        std::string status = stats->isUnderAttack ?
            mTools->getColoredString("ATTACKING", eColor::cyborgBlood) :
            mTools->getColoredString("Normal", eColor::lightGreen);
        row.push_back(status);

        rows.push_back(row);
    }
}

// Reset statistics
void CWFPFirewallModule::resetStatistics(const std::string& ipStr) {
    if (ipStr.empty()) {
        // Reset all statistics
        {
            std::lock_guard<std::mutex> lock(mStatsMutex);
            mAttackStats.clear();
        }

        // Reset metrics
        mMetrics.packetsProcessed = 0;
        mMetrics.packetsBlocked = 0;
        mMetrics.falsePositives = 0;
        mMetrics.processingTimeUs = 0;
        mMetrics.wfpOperations = 0;
        mMetrics.wfpErrors = 0;

        mTools->logEvent("Reset all attack statistics",
            "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);
    }
    else {
        // Reset specific IP
        IPAddress ip;
        bool isIPv6;
        if (stringToIPAddress(ipStr, ip, isIPv6)) {
            std::lock_guard<std::mutex> lock(mStatsMutex);
            mAttackStats.erase(ip);

            mTools->logEvent("Reset attack statistics for " + ipStr,
                "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);
        }
    }
}

// Convert string to IP address
bool CWFPFirewallModule::stringToIPAddress(const std::string& ipStr, IPAddress& ip, bool& isIPv6) {
    // Try IPv4 first
    struct in_addr addr4;
    if (inet_pton(AF_INET, ipStr.c_str(), &addr4) == 1) {
        ip.ipv4 = ntohl(addr4.s_addr);
        isIPv6 = false;
        return true;
    }

    // Try IPv6
    struct in6_addr addr6;
    if (inet_pton(AF_INET6, ipStr.c_str(), &addr6) == 1) {
        memcpy(ip.ipv6, addr6.s6_addr, 16);
        isIPv6 = true;
        return true;
    }

    return false;
}

// Convert IP address to string
std::string CWFPFirewallModule::ipAddressToString(const IPAddress& ip, bool isIPv6) {
    char buffer[INET6_ADDRSTRLEN] = { 0 };

    if (isIPv6) {
        struct in6_addr addr6;
        memcpy(addr6.s6_addr, ip.ipv6, 16);
        inet_ntop(AF_INET6, &addr6, buffer, INET6_ADDRSTRLEN);
    }
    else {
        struct in_addr addr4;
        addr4.s_addr = htonl(ip.ipv4);
        inet_ntop(AF_INET, &addr4, buffer, INET_ADDRSTRLEN);
    }

    return std::string(buffer);
}

// Log attack detected
void CWFPFirewallModule::logAttackDetected(const std::string& sourceIP, AttackType attackType) {
    std::stringstream msg;
    msg << "DoS attack detected from " << sourceIP << " - Type: ";

    std::vector<std::string> attackTypes;

    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::SynFlood)) {
        attackTypes.push_back("SYN Flood");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::UdpFlood)) {
        attackTypes.push_back("UDP Flood");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::IcmpFlood)) {
        attackTypes.push_back("ICMP Flood");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::ConnectionFlood)) {
        attackTypes.push_back("Connection Flood");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::PacketFlood)) {
        attackTypes.push_back("Packet Flood");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::SmallPacketFlood)) {
        attackTypes.push_back("Small Packet Flood");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::SlowlorisAttack)) {
        attackTypes.push_back("Slowloris");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::AmplificationAttack)) {
        attackTypes.push_back("Amplification");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::ZeroWindowAttack)) {
        attackTypes.push_back("Zero Window");
    }
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::LandAttack)) {
        attackTypes.push_back("Land Attack");
    }

    for (size_t i = 0; i < attackTypes.size(); ++i) {
        msg << attackTypes[i];
        if (i < attackTypes.size() - 1) {
            msg << ", ";
        }
    }

    mTools->logEvent(msg.str(), "WFP Firewall", eLogEntryCategory::network,
        10, eLogEntryType::warning, eColor::cyborgBlood);
}

// Notify network manager
void CWFPFirewallModule::notifyNetworkManager(const std::string& sourceIP, AttackType attackType) {
    // Update network manager's attack tracking
    if (static_cast<uint32_t>(attackType) & static_cast<uint32_t>(AttackType::SlowlorisAttack)) {
        // Increment HTTP connection attack counter
        //for (int i = 0; i < 10; ++i) {
        //    mNetworkManager->incrementHTTPConnection(sourceIP);
        // }
    }
}

// Set thresholds
bool CWFPFirewallModule::setThresholds(const DosThresholds& thresholds) {
    if (!thresholds.validate()) {
        mTools->logEvent("Invalid DoS thresholds provided",
            "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::warning);
        return false;
    }

    mThresholds = thresholds;

    mTools->logEvent("Updated DoS detection thresholds",
        "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);

    return true;
}

// Get thresholds
DosThresholds CWFPFirewallModule::getThresholds() const {
    return mThresholds;
}

// Set enabled state
void CWFPFirewallModule::setEnabled(bool enabled) {
    mEnabled = enabled;

    mTools->logEvent(std::string("WFP DoS Protection ") + (enabled ? "enabled" : "disabled"),
        "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification);
}

// Check if enabled
bool CWFPFirewallModule::isEnabled() const {
    return mEnabled;
}

// Get blocked IP count
uint64_t CWFPFirewallModule::getBlockedIPCount() const {
    std::lock_guard<std::mutex> lock(mBlockedIPsMutex);
    return mBlockedIPs.size();
}

// Get active attacks
std::vector<std::string> CWFPFirewallModule::getActiveAttacks() {
    std::vector<std::string> activeAttacks;
    std::lock_guard<std::mutex> lock(mStatsMutex);

    for (const auto& [ip, stats] : mAttackStats) {
        if (stats && stats->isUnderAttack) {
            activeAttacks.push_back(ipAddressToString(ip, false));
        }
    }

    return activeAttacks;
}

// Enable deep packet inspection
void CWFPFirewallModule::enableDeepPacketInspection(bool enable) {
    mDeepPacketInspection = enable;

    mTools->logEvent(std::string("Deep packet inspection ") + (enable ? "enabled" : "disabled"),
        "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);
}

// Enable behavioral analysis
void CWFPFirewallModule::enableBehavioralAnalysis(bool enable) {
    mBehavioralAnalysis = enable;

    mTools->logEvent(std::string("Behavioral analysis ") + (enable ? "enabled" : "disabled"),
        "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);
}

// Enable reputation-based filtering
void CWFPFirewallModule::enableReputationBasedFiltering(bool enable) {
    mReputationFiltering = enable;

    mTools->logEvent(std::string("Reputation-based filtering ") + (enable ? "enabled" : "disabled"),
        "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);
}

// Set whitelisted ports
void CWFPFirewallModule::setWhitelistedPorts(const std::vector<uint16_t>& ports) {
    std::lock_guard<std::mutex> lock(mWhitelistMutex);
    mWhitelistedPorts = ports;

    std::stringstream msg;
    msg << "Updated whitelisted ports: ";
    for (size_t i = 0; i < ports.size(); ++i) {
        msg << ports[i];
        if (i < ports.size() - 1) {
            msg << ", ";
        }
    }

    mTools->logEvent(msg.str(), "WFP Firewall", eLogEntryCategory::network,
        1, eLogEntryType::notification);
}

// Start network monitoring
bool CWFPFirewallModule::startNetworkMonitoring() {
    // Create event for shutdown signaling
    mEventHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!mEventHandle) {
        return false;
    }

    // Subscribe to WFP events
    if (!subscribeToWfpEvents()) {
        CloseHandle(mEventHandle);
        mEventHandle = nullptr;
        return false;
    }

    // Start monitoring thread
    mMonitoringActive = true;
    mMonitoringThread = std::thread(&CWFPFirewallModule::networkMonitoringThread, this);

    mTools->logEvent("Started network monitoring for DoS detection",
        "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);

    return true;
}

// Stop network monitoring
void CWFPFirewallModule::stopNetworkMonitoring() {
    mMonitoringActive = false;

    // Signal monitoring thread to stop
    if (mEventHandle) {
        SetEvent(mEventHandle);
    }

    // Unsubscribe from events
    if (mSubscriptionHandle && mEngineHandle) {
        FwpmNetEventUnsubscribe(mEngineHandle, mSubscriptionHandle);
        mSubscriptionHandle = nullptr;
    }

    // --- BEGIN FIX ---
    // Disable network events collection within a transaction.
    if (mEngineHandle) {
        WFPTransaction transaction(mEngineHandle);
        if (transaction.isValid()) {
            FWP_VALUE0 value = {  };
            value.type = FWP_UINT32;
            value.uint32 = 0;  // 0 to disable

            FwpmEngineSetOption(
                mEngineHandle,
                FWPM_ENGINE_COLLECT_NET_EVENTS,
                &value
            );
            // We don't strictly need to check the result here, as we are shutting down,
            // but we commit the transaction.
            transaction.commit();
        }
    }
    // --- END FIX ---

    // Close event handle
    if (mEventHandle) {
        CloseHandle(mEventHandle);
        mEventHandle = nullptr;
    }

    mTools->logEvent("Stopped network monitoring",
        "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);
}
// Subscribe to WFP network events
// In file WFPFirewallModule.cpp

// Subscribe to WFP network events
bool CWFPFirewallModule::subscribeToWfpEvents() {
    // --- BEGIN FIX ---
    // The call to FwpmEngineSetOption has been moved to initialize() where it is
    // correctly placed within a transaction. This function now only subscribes.
    // --- END FIX ---

    FWPM_NET_EVENT_SUBSCRIPTION subscription;
    memset(&subscription, 0, sizeof(FWPM_NET_EVENT_SUBSCRIPTION));

    FWPM_NET_EVENT_ENUM_TEMPLATE enumTemplate;
    memset(&enumTemplate, 0, sizeof(FWPM_NET_EVENT_ENUM_TEMPLATE));

    // We want all events - no filter conditions
    enumTemplate.numFilterConditions = 0;

    // Set up subscription
    subscription.enumTemplate = &enumTemplate;

    DWORD result = FwpmNetEventSubscribe(
        mEngineHandle,
        &subscription,
        nullptr,  // No callback - we'll poll
        nullptr,  // No context
        &mSubscriptionHandle
    );

    if (result != ERROR_SUCCESS) {
        if (result == FWP_E_NET_EVENTS_DISABLED) { // 0x8032001C
            mTools->logEvent("Network events are disabled in WFP. "
                "Will rely on IP Helper API monitoring instead.",
                "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::notification);
            return false;
        }
        else if (result == ERROR_ACCESS_DENIED) {
            mTools->logEvent("Access denied when subscribing to WFP events.",
                "WFP Firewall", eLogEntryCategory::network, 8, eLogEntryType::failure);
            return false;
        }
        else {
            mTools->logEvent("Failed to subscribe to WFP events: 0x" +
                std::to_string(result) + ". Will use alternative monitoring.",
                "WFP Firewall", eLogEntryCategory::network, 5, eLogEntryType::warning);
        }
        return false;
    }

    mTools->logEvent("Successfully subscribed to WFP network events",
        "WFP Firewall", eLogEntryCategory::network, 3, eLogEntryType::notification);

    return true;
}

// Network monitoring thread
void CWFPFirewallModule::networkMonitoringThread() {
    mTools->logEvent("Network monitoring thread started",
        "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);

    // Create a template for enumeration
    FWPM_NET_EVENT_ENUM_TEMPLATE enumTemplate = { 0 };
    HANDLE enumHandle = nullptr;

    while (mMonitoringActive && !mShutdownRequested) {
        // Poll for network events
        DWORD result = FwpmNetEventCreateEnumHandle(
            mEngineHandle,
            &enumTemplate,
            &enumHandle
        );

        if (result == ERROR_SUCCESS && enumHandle) {
            FWPM_NET_EVENT** events = nullptr;
            UINT32 numEvents = 0;

            // Get events (max 100 at a time)
            result = FwpmNetEventEnum(
                mEngineHandle,
                enumHandle,
                100,
                &events,
                &numEvents
            );

            if (result == ERROR_SUCCESS && events && numEvents > 0) {
                // Process each event
                for (UINT32 i = 0; i < numEvents; ++i) {
                    if (events[i]) {
                        processWfpEvent(events[i]);
                    }
                }

                // Free the events
                FwpmFreeMemory((void**)&events);
            }

            // Destroy enum handle
            FwpmNetEventDestroyEnumHandle(mEngineHandle, enumHandle);
        }

        // Also gather general network statistics
        gatherNetworkStatistics();

        // Wait before next poll
        WaitForSingleObject(mEventHandle, MONITORING_INTERVAL_MS);
    }

    mTools->logEvent("Network monitoring thread stopped",
        "WFP Firewall", eLogEntryCategory::network, 1, eLogEntryType::notification);
}

// Process WFP event
void CWFPFirewallModule::processWfpEvent(const FWPM_NET_EVENT* netEvent) {
    if (!netEvent || !mEnabled) {
        return;
    }

    // We're interested in connection events and drops
    switch (netEvent->type) {
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
        // Process drop events - these might indicate an attack
        if (netEvent->classifyDrop) {
            PacketInfo packetInfo;
            if (netEventToPacketInfo(netEvent, packetInfo)) {
                processIncomingPacket(packetInfo);
            }
        }
        break;

    case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
        // Process capability drop events
        // Note: The FWPM_NET_EVENT structure doesn't have a capabilityDrop member
        // This event type exists but doesn't provide additional drop info
        break;

    case FWPM_NET_EVENT_TYPE_IKEEXT_MM_FAILURE:
    case FWPM_NET_EVENT_TYPE_IKEEXT_QM_FAILURE:
    case FWPM_NET_EVENT_TYPE_IKEEXT_EM_FAILURE:
        // IKE failures might indicate attack
        break;

    case FWPM_NET_EVENT_TYPE_IPSEC_KERNEL_DROP:
        // IPsec drops
        if (netEvent->ipsecDrop) {
            PacketInfo packetInfo;
            if (netEventToPacketInfo(netEvent, packetInfo)) {
                processIncomingPacket(packetInfo);
            }
        }
        break;
    }
}

// Convert WFP net event to packet info
bool CWFPFirewallModule::netEventToPacketInfo(const FWPM_NET_EVENT* netEvent, PacketInfo& packetInfo) {
    if (!netEvent) {
        return false;
    }

    // Initialize packet info
    memset(&packetInfo, 0, sizeof(PacketInfo));
    packetInfo.timestamp = std::chrono::steady_clock::now();

    // Extract IP addresses based on event type
    if (netEvent->header.ipVersion == FWP_IP_VERSION_V4) {
        packetInfo.isIPv6 = false;

        if (netEvent->header.localAddrV4 != 0) {
            packetInfo.destIP.ipv4 = ntohl(netEvent->header.localAddrV4);
        }
        if (netEvent->header.remoteAddrV4 != 0) {
            packetInfo.sourceIP.ipv4 = ntohl(netEvent->header.remoteAddrV4);
        }
    }
    else if (netEvent->header.ipVersion == FWP_IP_VERSION_V6) {
        packetInfo.isIPv6 = true;

        if (netEvent->header.localAddrV6.byteArray16) {
            memcpy(packetInfo.destIP.ipv6, netEvent->header.localAddrV6.byteArray16, 16);
        }
        if (netEvent->header.remoteAddrV6.byteArray16) {
            memcpy(packetInfo.sourceIP.ipv6, netEvent->header.remoteAddrV6.byteArray16, 16);
        }
    }
    else {
        return false;  // Unknown IP version
    }

    // Extract ports
    packetInfo.sourcePort = netEvent->header.remotePort;
    packetInfo.destPort = netEvent->header.localPort;

    // Extract protocol
    packetInfo.protocol = netEvent->header.ipProtocol;

    // For drop events, we can get more info
    if (netEvent->type == FWPM_NET_EVENT_TYPE_CLASSIFY_DROP && netEvent->classifyDrop) {
        // Layer ID can help determine packet characteristics
        if (netEvent->classifyDrop->msFwpDirection == FWP_DIRECTION_INBOUND) {
            // This is an inbound packet
            packetInfo.packetSize = 0;  // Size not available in events

            // Check if it's a fragment
            packetInfo.isFragmented = false;  // Can't determine from event
        }
    }

    return true;
}

// Gather network statistics using IP Helper API
void CWFPFirewallModule::gatherNetworkStatistics() {
    // Use GetTcpStatisticsEx and GetUdpStatisticsEx to monitor for anomalies
    static MIB_TCPSTATS lastTcpStats = { 0 };
    static MIB_UDPSTATS lastUdpStats = { 0 };
    static bool initialized = false;

    MIB_TCPSTATS tcpStats;
    MIB_UDPSTATS udpStats;

    // Get TCP statistics
    if (GetTcpStatistics(&tcpStats) == NO_ERROR) {
        if (initialized) {
            // Check for SYN flood indicators
            DWORD synDelta = tcpStats.dwAttemptFails - lastTcpStats.dwAttemptFails;
            DWORD resetDelta = tcpStats.dwEstabResets - lastTcpStats.dwEstabResets;

            // If we see a spike in failed attempts or resets, it might indicate an attack
            if (synDelta > 1000 || resetDelta > 500) {
                mTools->logEvent("Detected abnormal TCP statistics - possible SYN flood",
                    "WFP Firewall", eLogEntryCategory::network, 7, eLogEntryType::warning);

                // We can't get source IPs from these stats, but we can increase vigilance
                // by temporarily lowering thresholds
            }
        }
        lastTcpStats = tcpStats;
    }

    // Get UDP statistics
    if (GetUdpStatistics(&udpStats) == NO_ERROR) {
        if (initialized) {
            DWORD udpErrorDelta = udpStats.dwInErrors - lastUdpStats.dwInErrors;
            DWORD udpDropDelta = udpStats.dwNoPorts - lastUdpStats.dwNoPorts;

            // High UDP errors might indicate UDP flood
            if (udpErrorDelta > 1000 || udpDropDelta > 1000) {
                mTools->logEvent("Detected abnormal UDP statistics - possible UDP flood",
                    "WFP Firewall", eLogEntryCategory::network, 7, eLogEntryType::warning);
            }
        }
        lastUdpStats = udpStats;
    }

    // Also check TCP connection table for connection flood detection
    PMIB_TCPTABLE tcpTable = nullptr;
    DWORD size = 0;

    // Get size needed
    if (GetTcpTable(tcpTable, &size, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
        tcpTable = (PMIB_TCPTABLE)malloc(size);
        if (tcpTable) {
            if (GetTcpTable(tcpTable, &size, TRUE) == NO_ERROR) {
                // Count connections per remote IP
                std::unordered_map<DWORD, int> connectionCounts;

                for (DWORD i = 0; i < tcpTable->dwNumEntries; ++i) {
                    if (tcpTable->table[i].dwState == MIB_TCP_STATE_SYN_RCVD) {
                        connectionCounts[tcpTable->table[i].dwRemoteAddr]++;
                    }
                }

                // Check for IPs with too many half-open connections
                for (const auto& [remoteAddr, count] : connectionCounts) {
                    if (count > 50) {  // Threshold for half-open connections
                        // Create packet info for this suspicious IP
                        PacketInfo packetInfo;
                        packetInfo.sourceIP.ipv4 = ntohl(remoteAddr);
                        packetInfo.isIPv6 = false;
                        packetInfo.protocol = IPPROTO_TCP;
                        packetInfo.tcpFlags = 0x02;  // SYN
                        packetInfo.timestamp = std::chrono::steady_clock::now();

                        // Process multiple times to trigger detection
                        for (int j = 0; j < count; ++j) {
                            processIncomingPacket(packetInfo);
                        }
                    }
                }
            }
            free(tcpTable);
        }
    }

    // Analyze TCP connections in detail
    analyzeTcpConnections();

    // Analyze UDP endpoints for UDP flood
    analyzeUdpEndpoints();

    initialized = true;
}

// Analyze TCP connections for more detailed attack detection
void CWFPFirewallModule::analyzeTcpConnections() {
    PMIB_TCPTABLE tcpTable = nullptr;
    ULONG size = 0;

    // Get extended TCP table
    // Using TCP_TABLE_BASIC_LISTENER enum value
    if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_BASIC_LISTENER, 0) == ERROR_INSUFFICIENT_BUFFER) {
        tcpTable = (PMIB_TCPTABLE)malloc(size);
        if (tcpTable) {
            if (GetExtendedTcpTable(tcpTable, &size, FALSE, AF_INET, TCP_TABLE_BASIC_LISTENER, 0) == NO_ERROR) {
                // Analyze connection patterns
                std::unordered_map<DWORD, std::vector<DWORD>> ipToStates;

                for (DWORD i = 0; i < tcpTable->dwNumEntries; ++i) {
                    ipToStates[tcpTable->table[i].dwRemoteAddr].push_back(tcpTable->table[i].dwState);
                }

                // Look for attack patterns
                for (const auto& [remoteAddr, states] : ipToStates) {
                    int synCount = 0;
                    int establishedCount = 0;

                    for (DWORD state : states) {
                        if (state == MIB_TCP_STATE_SYN_RCVD) synCount++;
                        else if (state == MIB_TCP_STATE_ESTAB) establishedCount++;
                    }

                    // High ratio of SYN to established might indicate SYN flood
                    if (synCount > 20 && synCount > establishedCount * 10) {
                        PacketInfo packetInfo;
                        packetInfo.sourceIP.ipv4 = ntohl(remoteAddr);
                        packetInfo.isIPv6 = false;
                        packetInfo.protocol = IPPROTO_TCP;
                        packetInfo.tcpFlags = 0x02;  // SYN
                        packetInfo.timestamp = std::chrono::steady_clock::now();

                        // Simulate multiple SYN packets
                        for (int j = 0; j < synCount; ++j) {
                            processIncomingPacket(packetInfo);
                        }
                    }
                }
            }
            free(tcpTable);
        }
    }
}

// Analyze UDP endpoints for UDP flood detection
void CWFPFirewallModule::analyzeUdpEndpoints() {
    PMIB_UDPTABLE_OWNER_PID udpTable = nullptr;
    DWORD size = 0;

    // Get UDP table
    // Using UDP_TABLE_OWNER_PID enum value
    if (GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER) {
        udpTable = (PMIB_UDPTABLE_OWNER_PID)malloc(size);
        if (udpTable) {
            if (GetExtendedUdpTable(udpTable, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
                // Count UDP endpoints per local port
                std::unordered_map<DWORD, int> portCounts;

                for (DWORD i = 0; i < udpTable->dwNumEntries; ++i) {
                    DWORD port = ntohs((u_short)udpTable->table[i].dwLocalPort);
                    portCounts[port]++;
                }

                // Look for ports with excessive bindings (potential amplification attack targets)
                for (const auto& [port, count] : portCounts) {
                    if (count > 100) {  // Unusual number of bindings on same port
                        mTools->logEvent("Detected excessive UDP bindings on port " + std::to_string(port) +
                            " - potential amplification attack vector",
                            "WFP Firewall", eLogEntryCategory::network, 6, eLogEntryType::warning);
                    }
                }
            }
            free(udpTable);
        }
    }
}