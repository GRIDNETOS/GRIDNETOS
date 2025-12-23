// DllInjectionProtection.cpp
#include "DllInjectionProtection.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

namespace Security {

    // Default blocked DLLs database
    static const std::vector<BlockedDllEntry> DEFAULT_BLOCKED_DLLS = {
        {
            L"RTSSHooks64.dll",
            L"RivaTuner Statistics Server",
            L"RivaTuner/MSI Afterburner overlay software is incompatible with this application.\n"
            L"It may cause crashes and exception handling failures.\n\n"
            L"Please close RivaTuner/MSI Afterburner before continuing.",
            BlockingPolicy::ATTEMPT_UNLOAD
        },
        {
            L"RTSSHooks.dll",
            L"RivaTuner Statistics Server (32-bit)",
            L"RivaTuner overlay software detected.",
            BlockingPolicy::ATTEMPT_UNLOAD
        },
        {
            L"fraps64.dll",
            L"Fraps",
            L"Fraps overlay software may cause compatibility issues.",
            BlockingPolicy::WARN_ONLY
        },
        {
            L"igdumdim64.dll",
            L"Intel Graphics Injector",
            L"Intel Graphics overlay detected. This may impact performance.",
            BlockingPolicy::WARN_ONLY
        },
        {
            L"OBSHook64.dll",
            L"OBS Studio",
            L"OBS Studio game capture detected.",
            BlockingPolicy::WARN_ONLY
        },
        {
            L"DiscordHook64.dll",
            L"Discord Overlay",
            L"Discord overlay detected. This may impact performance.",
            BlockingPolicy::WARN_ONLY
        }
    };

    bool DllInjectionProtection::Initialize(const Configuration& config) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_protectionActive.load()) {
            Log(L"Protection already initialized");
            return true;
        }

        m_config = config;

        // Initialize logging
        if (m_config.enableLogging && !m_config.logFilePath.empty()) {
            m_logFile = CreateFileW(
                m_config.logFilePath.c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                nullptr,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
        }

        Log(L"=== DLL Injection Protection Initializing ===");
        Log(L"Protection Level: " + std::to_wstring(static_cast<int>(m_config.level)));

        // Check for override flag
        if (m_config.allowOverride) {
            LPWSTR cmdLine = GetCommandLineW();
            if (wcsstr(cmdLine, L"--allow-injection")) {
                Log(L"Protection bypassed via --allow-injection flag");
                m_protectionActive = true;
                return true;
            }
        }

        // Initialize blocked DLLs list
        m_blockedDlls = DEFAULT_BLOCKED_DLLS;
        m_blockedDlls.insert(m_blockedDlls.end(),
            m_config.additionalBlockedDlls.begin(),
            m_config.additionalBlockedDlls.end());

        // Step 1: Check for already loaded blocked DLLs
        if (!CheckForBlockedDlls()) {
            Log(L"Failed to handle blocked DLLs", true);
            if (m_config.level == ProtectionLevel::MAXIMUM) {
                return false;
            }
        }

        // Step 2: Enable mitigation policies
        bool mitigationsEnabled = false;
        switch (m_config.level) {
        case ProtectionLevel::MAXIMUM:
            mitigationsEnabled = EnableAdvancedMitigations();
            break;
        case ProtectionLevel::STANDARD:
            mitigationsEnabled = EnableMitigationPolicies();
            break;
        case ProtectionLevel::MINIMAL:
            mitigationsEnabled = true;  // Skip mitigations in minimal mode
            break;
        }

        if (!mitigationsEnabled) {
            Log(L"Failed to enable some mitigation policies", true);
            // Continue anyway unless maximum protection
            if (m_config.level == ProtectionLevel::MAXIMUM) {
                return false;
            }
        }

        // Step 3: Secure DLL search path
        SecureDllSearchPath();

        // Step 4: Start monitoring thread (except in minimal mode)
        if (m_config.level != ProtectionLevel::MINIMAL) {
            m_monitorThread = CreateThread(
                nullptr, 0,
                MonitoringThread,
                this,
                0, nullptr
            );

            if (!m_monitorThread) {
                Log(L"Failed to create monitoring thread", true);
            }
            else {
                SetThreadPriority(m_monitorThread, THREAD_PRIORITY_LOWEST);
            }
        }

        m_protectionActive = true;
        Log(L"=== Protection Initialized Successfully ===");

        return true;
    }

    bool DllInjectionProtection::EnableMitigationPolicies() {
        Log(L"Enabling standard mitigation policies");
        bool allSucceeded = true;

        // 1. Binary Signature Policy - Only signed DLLs
        PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY sigPolicy = {};
        sigPolicy.MicrosoftSignedOnly = 1;

        if (!SetProcessMitigationPolicy(ProcessSignaturePolicy,
            &sigPolicy, sizeof(sigPolicy))) {
            // Try less restrictive policy
            sigPolicy.MicrosoftSignedOnly = 0;
            sigPolicy.StoreSignedOnly = 1;

            if (!SetProcessMitigationPolicy(ProcessSignaturePolicy,
                &sigPolicy, sizeof(sigPolicy))) {
                Log(L"Failed to set signature policy: " +
                    std::to_wstring(GetLastError()), true);
                allSucceeded = false;
            }
        }

        // 2. Extension Point Disable Policy
        PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extPolicy = {};
        extPolicy.DisableExtensionPoints = 1;

        if (!SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy,
            &extPolicy, sizeof(extPolicy))) {
            Log(L"Failed to disable extension points: " +
                std::to_wstring(GetLastError()), true);
            allSucceeded = false;
        }

        // 3. Image Load Policy - Prevent remote and low-integrity images
        PROCESS_MITIGATION_IMAGE_LOAD_POLICY imagePolicy = {};
        imagePolicy.NoRemoteImages = 1;
        imagePolicy.NoLowMandatoryLabelImages = 1;

        if (!SetProcessMitigationPolicy(ProcessImageLoadPolicy,
            &imagePolicy, sizeof(imagePolicy))) {
            Log(L"Failed to set image load policy: " +
                std::to_wstring(GetLastError()), true);
            allSucceeded = false;
        }

        return allSucceeded;
    }

    bool DllInjectionProtection::EnableAdvancedMitigations() {
        Log(L"Enabling maximum mitigation policies");

        // Start with standard mitigations
        bool result = EnableMitigationPolicies();

        // Add strict mitigations
        PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynPolicy = {};
        dynPolicy.ProhibitDynamicCode = 1;

        if (!SetProcessMitigationPolicy(ProcessDynamicCodePolicy,
            &dynPolicy, sizeof(dynPolicy))) {
            Log(L"Failed to prohibit dynamic code (non-critical)", true);
        }

        // Strict handle check
        PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handlePolicy = {};
        handlePolicy.RaiseExceptionOnInvalidHandleReference = 1;
        handlePolicy.HandleExceptionsPermanentlyEnabled = 1;

        if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy,
            &handlePolicy, sizeof(handlePolicy))) {
            Log(L"Failed to enable strict handle checks", true);
        }

        return result;
    }

    void DllInjectionProtection::SecureDllSearchPath() {
        Log(L"Securing DLL search path");

        // Remove current directory from search
        SetDllDirectoryW(L"");

        // Restrict to system and application directories only
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 |
            LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    }

    bool DllInjectionProtection::CheckForBlockedDlls() {
        Log(L"Scanning for blocked DLLs");

        for (const auto& entry : m_blockedDlls) {
            HMODULE hModule = GetModuleHandleW(entry.dllName.c_str());
            if (hModule) {
                Log(L"Detected blocked DLL: " + entry.displayName);
                m_detectedDlls.insert(entry.dllName);

                if (!HandleBlockedDll(entry, hModule)) {
                    if (entry.policy == BlockingPolicy::TERMINATE_IF_PRESENT) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool DllInjectionProtection::HandleBlockedDll(const BlockedDllEntry& entry,
        HMODULE hModule) {
        // First try custom handler if provided
        if (entry.customHandler) {
            Log(L"Executing custom handler for " + entry.displayName);
            return entry.customHandler();
        }

        switch (entry.policy) {
        case BlockingPolicy::WARN_ONLY:
            if (m_config.showUI) {
                ShowBlockedDllDialog(entry);
            }
            return true;

        case BlockingPolicy::ATTEMPT_UNLOAD:
            if (m_config.showUI && !ShowBlockedDllDialog(entry)) {
                return false;
            }
            return ForceUnloadDll(entry.dllName);

        case BlockingPolicy::TERMINATE_IF_PRESENT:
            if (m_config.showUI) {
                MessageBoxW(nullptr,
                    (entry.warningMessage + L"\n\nThe application will now exit.").c_str(),
                    L"Incompatible Software Detected",
                    MB_OK | MB_ICONERROR);
            }
            ExitProcess(1);
            return false;
        }

        return true;
    }

    bool DllInjectionProtection::ForceUnloadDll(const std::wstring& dllName) {
        Log(L"Attempting to unload: " + dllName);

        HMODULE hModule = GetModuleHandleW(dllName.c_str());
        if (!hModule) {
            return true;  // Already unloaded
        }

        // Try up to 10 times to unload
        int attempts = 0;
        while (hModule && attempts < 10) {
            if (!FreeLibrary(hModule)) {
                break;
            }
            attempts++;
            hModule = GetModuleHandleW(dllName.c_str());
        }

        bool success = (hModule == nullptr);
        Log(L"Unload " + dllName + L": " + (success ? L"SUCCESS" : L"FAILED"));

        return success;
    }

    bool DllInjectionProtection::ShowBlockedDllDialog(const BlockedDllEntry& entry) {
        std::wstring message = entry.warningMessage;

        if (entry.policy == BlockingPolicy::ATTEMPT_UNLOAD) {
            message += L"\n\nClick OK to attempt automatic removal and continue.";
            message += L"\nClick Cancel to exit the application.";

            int result = MessageBoxW(nullptr, message.c_str(),
                (L"Warning: " + entry.displayName).c_str(),
                MB_OKCANCEL | MB_ICONWARNING);
            return (result == IDOK);
        }
        else {
            message += L"\n\nContinuing may cause instability.";
            MessageBoxW(nullptr, message.c_str(),
                (L"Warning: " + entry.displayName).c_str(),
                MB_OK | MB_ICONWARNING);
            return true;
        }
    }

    DWORD WINAPI DllInjectionProtection::MonitoringThread(LPVOID param) {
        auto* protection = static_cast<DllInjectionProtection*>(param);
        protection->MonitorForInjections();
        return 0;
    }

    void DllInjectionProtection::MonitorForInjections() {
        Log(L"Monitoring thread started");

        while (!m_stopMonitoring.load()) {
            Sleep(1000);  // Check every second

            // Re-scan for blocked DLLs
            for (const auto& entry : m_blockedDlls) {
                HMODULE hModule = GetModuleHandleW(entry.dllName.c_str());

                if (hModule && m_detectedDlls.find(entry.dllName) == m_detectedDlls.end()) {
                    // New injection detected
                    Log(L"Runtime injection detected: " + entry.displayName, true);
                    m_detectedDlls.insert(entry.dllName);

                    // Handle based on policy
                    HandleBlockedDll(entry, hModule);
                }
            }
        }

        Log(L"Monitoring thread stopped");
    }

    void DllInjectionProtection::Log(const std::wstring& message, bool isError) {
        if (!m_config.enableLogging) return;

        // Get timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::wstringstream ss;
        ss << L"[" << std::put_time(std::localtime(&time_t), L"%Y-%m-%d %H:%M:%S") << L"] ";
        ss << (isError ? L"[ERROR] " : L"[INFO] ");
        ss << message << L"\r\n";

        std::wstring logLine = ss.str();

        // Write to file if available
        if (m_logFile != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(m_logFile, logLine.c_str(),
                static_cast<DWORD>(logLine.length() * sizeof(wchar_t)),
                &written, nullptr);
            FlushFileBuffers(m_logFile);
        }

        // Also output to debug console
        OutputDebugStringW(logLine.c_str());
    }

    void DllInjectionProtection::AddBlockedDll(const BlockedDllEntry& entry) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_blockedDlls.push_back(entry);
        Log(L"Added blocked DLL: " + entry.displayName);
    }

    std::vector<std::wstring> DllInjectionProtection::ScanForInjectedDlls() {
        std::vector<std::wstring> injected;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            GetCurrentProcessId());
        if (snapshot == INVALID_HANDLE_VALUE) {
            return injected;
        }

        MODULEENTRY32W me32 = {};
        me32.dwSize = sizeof(me32);

        if (Module32FirstW(snapshot, &me32)) {
            do {
                std::wstring modulePath = me32.szExePath;

                // Check if module is from system directory
                wchar_t systemDir[MAX_PATH];
                GetSystemDirectoryW(systemDir, MAX_PATH);

                wchar_t windowsDir[MAX_PATH];
                GetWindowsDirectoryW(windowsDir, MAX_PATH);

                wchar_t appDir[MAX_PATH];
                GetModuleFileNameW(nullptr, appDir, MAX_PATH);
                PathRemoveFileSpecW(appDir);

                // If not from trusted location, it might be injected
                if (modulePath.find(systemDir) == std::wstring::npos &&
                    modulePath.find(windowsDir) == std::wstring::npos &&
                    modulePath.find(appDir) == std::wstring::npos) {
                    injected.push_back(me32.szModule);
                }
            } while (Module32NextW(snapshot, &me32));
        }

        CloseHandle(snapshot);
        return injected;
    }

    std::wstring DllInjectionProtection::GetStatusReport() const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::wstringstream report;
        report << L"=== DLL Injection Protection Status ===\n";
        report << L"Active: " << (m_protectionActive.load() ? L"Yes" : L"No") << L"\n";
        report << L"Protection Level: ";

        switch (m_config.level) {
        case ProtectionLevel::MINIMAL: report << L"Minimal\n"; break;
        case ProtectionLevel::STANDARD: report << L"Standard\n"; break;
        case ProtectionLevel::MAXIMUM: report << L"Maximum\n"; break;
        }

        report << L"Monitoring Thread: " << (m_monitorThread ? L"Running" : L"Not Running") << L"\n";
        report << L"Detected Blocked DLLs: " << m_detectedDlls.size() << L"\n";

        if (!m_detectedDlls.empty()) {
            report << L"List:\n";
            for (const auto& dll : m_detectedDlls) {
                report << L"  - " << dll << L"\n";
            }
        }

        return report.str();
    }

    DllInjectionProtection::~DllInjectionProtection() {
        m_stopMonitoring = true;

        if (m_monitorThread) {
            WaitForSingleObject(m_monitorThread, 2000);
            CloseHandle(m_monitorThread);
        }

        if (m_logFile != INVALID_HANDLE_VALUE) {
            Log(L"=== Protection Shutting Down ===");
            CloseHandle(m_logFile);
        }
    }

} // namespace Security