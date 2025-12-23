/**
 * CPU Feature Detection Test
 */

#include <iostream>
#include <intrin.h>

void checkCPUFeatures() {
    int cpuInfo[4];

    // Check basic features
    __cpuid(cpuInfo, 0);
    int nIds = cpuInfo[0];

    std::cout << "CPU Feature Check:" << std::endl;

    if (nIds >= 1) {
        __cpuid(cpuInfo, 1);
        bool SSE2 = (cpuInfo[3] & (1 << 26)) != 0;
        bool SSE3 = (cpuInfo[2] & (1 << 0)) != 0;
        bool SSE41 = (cpuInfo[2] & (1 << 19)) != 0;
        bool SSE42 = (cpuInfo[2] & (1 << 20)) != 0;
        bool AVX = (cpuInfo[2] & (1 << 28)) != 0;

        std::cout << "  SSE2:  " << (SSE2 ? "Yes" : "No") << std::endl;
        std::cout << "  SSE3:  " << (SSE3 ? "Yes" : "No") << std::endl;
        std::cout << "  SSE4.1: " << (SSE41 ? "Yes" : "No") << std::endl;
        std::cout << "  SSE4.2: " << (SSE42 ? "Yes" : "No") << std::endl;
        std::cout << "  AVX:   " << (AVX ? "Yes" : "No") << std::endl;
    }

    if (nIds >= 7) {
        __cpuidex(cpuInfo, 7, 0);
        bool AVX2 = (cpuInfo[1] & (1 << 5)) != 0;
        bool AVX512F = (cpuInfo[1] & (1 << 16)) != 0;
        bool AVX512BW = (cpuInfo[1] & (1 << 30)) != 0;

        std::cout << "  AVX2:  " << (AVX2 ? "Yes" : "No") << std::endl;
        std::cout << "  AVX512F: " << (AVX512F ? "Yes" : "No") << std::endl;
        std::cout << "  AVX512BW: " << (AVX512BW ? "Yes" : "No") << std::endl;

        if (AVX512F) {
            std::cout << "\n*** THIS MACHINE HAS AVX512! ***" << std::endl;
            std::cout << "The bug may be in the AVX512 encoder, not AVX2!" << std::endl;
        }
    }
}

int main() {
    checkCPUFeatures();
    return 0;
}
