// DllInjectionProtection.h
#pragma once

#include <windows.h>
#include <processthreadsapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

namespace Security {

    enum class BlockingPolicy {
        WARN_ONLY,           // Just warn the user
        ATTEMPT_UNLOAD,      // Try to unload the DLL
        TERMINATE_IF_PRESENT // Terminate if DLL is present
    };

    enum class ProtectionLevel {
        MINIMAL,    // Basic protection
        STANDARD,   // Recommended for most applications  
        MAXIMUM     // Maximum security (may break some legitimate functionality)
    };

    struct BlockedDllEntry {
        std::wstring dllName;
        std::wstring displayName;
        std::wstring warningMessage;
        BlockingPolicy policy;
        std::function<bool()> customHandler;  // Optional custom handler

        BlockedDllEntry(const std::wstring& dll,
            const std::wstring& display,
            const std::wstring& warning,
            BlockingPolicy pol = BlockingPolicy::ATTEMPT_UNLOAD)
            : dllName(dll), displayName(display), warningMessage(warning), policy(pol) {
        }
    };

    class DllInjectionProtection {
    public:
        struct Configuration {
            ProtectionLevel level = ProtectionLevel::STANDARD;
            bool enableLogging = true;
            bool showUI = true;
            bool allowOverride = false;  // Allow --allow-injection flag
            std::wstring logFilePath = L"injection_protection.log";
            std::vector<BlockedDllEntry> additionalBlockedDlls;
        };

        // Singleton pattern for global protection
        static DllInjectionProtection& Instance() {
            static DllInjectionProtection instance;
            return instance;
        }

        // Main initialization - call this first thing in main()
        bool Initialize(const Configuration& config = Configuration{});

        // Add additional DLLs to block at runtime
        void AddBlockedDll(const BlockedDllEntry& entry);

        // Check current protection status
        bool IsProtectionActive() const { return m_protectionActive.load(); }

        // Get detailed status report
        std::wstring GetStatusReport() const;

        // Manual scan for injected DLLs
        std::vector<std::wstring> ScanForInjectedDlls();

        // Destructor
        ~DllInjectionProtection();

    private:
        DllInjectionProtection() = default;
        DllInjectionProtection(const DllInjectionProtection&) = delete;
        DllInjectionProtection& operator=(const DllInjectionProtection&) = delete;

        // Core protection methods
        bool EnableMitigationPolicies();
        bool EnableAdvancedMitigations();
        void SecureDllSearchPath();
        bool CheckForBlockedDlls();
        bool HandleBlockedDll(const BlockedDllEntry& entry, HMODULE hModule);
        bool ForceUnloadDll(const std::wstring& dllName);

        // Monitoring thread
        static DWORD WINAPI MonitoringThread(LPVOID param);
        void MonitorForInjections();

        // Logging
        void Log(const std::wstring& message, bool isError = false);

        // UI helpers
        bool ShowBlockedDllDialog(const BlockedDllEntry& entry);

        // State
        Configuration m_config;
        std::atomic<bool> m_protectionActive{ false };
        std::atomic<bool> m_stopMonitoring{ false };
        HANDLE m_monitorThread{ nullptr };
        mutable std::mutex m_mutex;
        std::vector<BlockedDllEntry> m_blockedDlls;
        std::unordered_set<std::wstring> m_detectedDlls;
        HANDLE m_logFile{ INVALID_HANDLE_VALUE };
    };

} // namespace Security