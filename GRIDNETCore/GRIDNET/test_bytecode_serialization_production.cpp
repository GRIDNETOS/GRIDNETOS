/**
 * @file test_bytecode_serialization_production.cpp
 * @brief Production GridScript bytecode serialization cross-validation tests
 *
 * This test uses the PRODUCTION CGridScriptCompiler with its standalone constructor.
 * It requires linking against GRIDNET libraries but can run without the full GRIDNET Core.
 *
 * The standalone constructor initializes:
 * - CTools with minimal dependencies (LocalData mode, no DTI)
 * - CScriptEngine with nullptr TransactionManager and BlockchainManager
 * - All codeword definitions via reset(true) -> initializeDefinitions() -> definePrimitives()
 *
 * Tests:
 * 1. C++ standalone round-trip tests (compile -> decompile -> verify)
 * 2. JavaScript bytecode cross-validation (decompile JS bytecode in C++)
 * 3. Production test cases (Token Pool Registration, Sacrificial Transaction)
 * 4. Reverse cross-validation output (C++ bytecode for JavaScript to test)
 *
 * Build: Compile as part of GRIDNET solution or link against GridScript, Tools, CryptoFactory libs
 */

#include "pch.h"
#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "GridScriptCompiler.h"
#include "tools.h"

// ============================================================================
// Test harness
// ============================================================================
struct TestCase {
    std::string name;
    std::string input;
    bool shouldCompile;
    bool shouldRoundTrip;
};

struct CrossValidationTest {
    std::string name;
    std::string jsBase64Bytecode;
    std::string expectedDecompiled;
};

int main() {
    std::cout << "=== GridScript Bytecode Serialization Production Tests ===" << std::endl;
    std::cout << "Using production CGridScriptCompiler with standalone constructor" << std::endl;
    std::cout << std::endl;

    // Create production compiler with standalone constructor (V2 bytecode)
    std::cout << "Initializing production compiler (V2 bytecode)..." << std::endl;
    auto compiler = std::make_shared<CGridScriptCompiler>(2);
    auto tools = CTools::getInstance();

    int passed = 0;
    int failed = 0;

    // ════════════════════════════════════════════════════════════════════════════
    // SECTION 1: C++ STANDALONE ROUND-TRIP TESTS
    // ════════════════════════════════════════════════════════════════════════════
    std::cout << std::endl;
    std::cout << "--- Section 1: C++ Standalone Round-Trip Tests ---" << std::endl;

    std::vector<TestCase> roundTripTests = {
        // Basic commands
        {"simple cd", "cd /test/path", true, true},
        {"cd with address", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo", true, true},
        {"cd with /", "cd /", true, true},
        {"multiple cd", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd /", true, true},
        {"simple data", "data GNC", true, true},
        {"multiple data", "data GNC data test", true, true},
        {"xvalue command", "data GNC xvalue", true, true},

        // data58 tests (uses base58Check encoding with single SHA256 checksum)
        {"simple data58", "data58 2bNcNLCKuxzGWJ5VP", true, true},

        // regPool tests (V2 allows omitting inline args)
        {"regPool without inline", "regPool", true, true},

        // Production test cases
        {"Token Pool Registration", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool", true, true},
        {"Sacrificial Transaction", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000", true, true},
    };

    for (const auto& test : roundTripTests) {
        std::vector<uint8_t> bytecode;
        bool compileResult = compiler->compile(test.input, bytecode);

        if (compileResult != test.shouldCompile) {
            std::cout << "[FAIL] " << test.name << " - Compile "
                      << (compileResult ? "succeeded" : "failed")
                      << " but expected " << (test.shouldCompile ? "success" : "failure") << std::endl;
            failed++;
            continue;
        }

        if (!compileResult) {
            std::cout << "[PASS] " << test.name << " - Compile correctly failed" << std::endl;
            passed++;
            continue;
        }

        std::string decompiled;
        bool decompileResult = compiler->decompile(bytecode, decompiled);

        if (!decompileResult) {
            std::cout << "[FAIL] " << test.name << " - Decompilation failed" << std::endl;
            failed++;
            continue;
        }

        if (test.shouldRoundTrip) {
            if (test.input == decompiled) {
                std::cout << "[PASS] " << test.name << std::endl;
                passed++;
            } else {
                std::cout << "[FAIL] " << test.name << " - Round-trip mismatch" << std::endl;
                std::cout << "  Expected: " << test.input.substr(0, 80) << (test.input.size() > 80 ? "..." : "") << std::endl;
                std::cout << "  Got:      " << decompiled.substr(0, 80) << (decompiled.size() > 80 ? "..." : "") << std::endl;
                failed++;
            }
        } else {
            std::cout << "[PASS] " << test.name << " - Compile succeeded" << std::endl;
            passed++;
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // SECTION 2: JAVASCRIPT CROSS-VALIDATION
    // Decompile bytecode generated by JavaScript compiler
    // ════════════════════════════════════════════════════════════════════════════
    std::cout << std::endl;
    std::cout << "--- Section 2: JavaScript Cross-Validation (JS->C++) ---" << std::endl;

    std::vector<CrossValidationTest> jsTests = {
        // Basic cd commands
        {"JS: simple cd", "whAk6g6VErtzg56RW8Sswpg4Ck9KB6XbBrPgrwsDF8usgKIKL3Rlc3QvcGF0aA==", "cd /test/path"},
        {"JS: cd with address", "whAk6g6VErtzg56RW8Sswpg4Ck9KB6XbBrPgrwsDF8usgKIiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqbw==", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo"},
        {"JS: cd /", "whAk6g6VErtzg56RW8Sswpg4Ck9KB6XbBrPgrwsDF8usgKIBLw==", "cd /"},

        // Basic data commands
        {"JS: simple data", "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YToCwNHTkM=", "data GNC"},

        // data58 tests
        {"JS: simple data58", "wiog+86lvQXJR+wVKllsyDlF/kWoZZTLReDRne1pWn8YgI4IVGVzdERhdGE=", "data58 2bNcNLCKuxzGWJ5VP"},

        // regPool tests
        {"JS: regPool empty", "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgSoA", "regPool"},

        // Production test cases
        {"JS: Token Pool Registration", "wtzjzPexEv4/8yH8S+oTujd01nGO8kxZDDx7twCCKzOCgKIjLzE0NGMyQW9mNXQzR0RVVHY4eDltUG9ycHpNTGhrRlFMV3CAjoILATCCAQcCAQIwggEABCIxNDRjMkFvZjV0M0dEVVR2OHg5bVBvcnB6TUxoa0ZRTFdwBAAEASICAQMEA8CjmwQhAXpHO7GBQNZaQ8hKoM4y2rPMzzRUO2aY3SJgRPzxJJ9EBCCBgKTdmaGlc9X0nTF3F7UVYmdDoi2yzYu9NfbOx5n8KgQIcGF5bWVudHMCAQAwezAnBCAJfthldb8KyiENmlK/k/jSv/c7u/Q7yZfPjoAEHHbmhAQABAEAMCcEILJzy6vRPvyf0vHh6UaAi5EEXhhGuXJmVLOFSrSYsIu6BAAEAQAwJwQgtgI1MxoQOwoAz3R8uLfapYcZZdCcsZBxSSBW8EwH1joEAAQBAIEqAA==",
            "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool"},
        {"JS: Sacrificial Transaction", "wnSx3I5FOWDFLG7pdMYdqI9f6oPQKDvH8t9pOKZDXPeEgKIjLzE0NGMyQW9mNXQzR0RVVHY4eDltUG9ycHpNTGhrRlFMV3CAywgxNDQwMDAwMA==",
            "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000"},
    };

    for (const auto& test : jsTests) {
        // Decode base64 bytecode
        std::vector<uint8_t> jsBytecode;
        if (!tools->base64Decode(test.jsBase64Bytecode, jsBytecode)) {
            std::cout << "[FAIL] " << test.name << " - Base64 decode failed" << std::endl;
            failed++;
            continue;
        }

        std::string decompiled;
        if (!compiler->decompile(jsBytecode, decompiled)) {
            std::cout << "[FAIL] " << test.name << " - Decompilation failed" << std::endl;
            failed++;
            continue;
        }

        if (test.expectedDecompiled == decompiled) {
            std::cout << "[PASS] " << test.name << std::endl;
            passed++;
        } else {
            std::cout << "[FAIL] " << test.name << " - Decompilation mismatch" << std::endl;
            std::cout << "  Expected: " << test.expectedDecompiled.substr(0, 80) << (test.expectedDecompiled.size() > 80 ? "..." : "") << std::endl;
            std::cout << "  Got:      " << decompiled.substr(0, 80) << (decompiled.size() > 80 ? "..." : "") << std::endl;
            failed++;
        }
    }

    // ════════════════════════════════════════════════════════════════════════════
    // SECTION 3: REVERSE CROSS-VALIDATION OUTPUT
    // Generate C++ bytecode for JavaScript to test
    // ════════════════════════════════════════════════════════════════════════════
    std::cout << std::endl;
    std::cout << "--- Section 3: C++ Bytecode for JavaScript Testing ---" << std::endl;
    std::cout << "Copy these test vectors to JavaScript test file:" << std::endl;
    std::cout << std::endl;

    std::vector<std::pair<std::string, std::string>> reverseTests = {
        {"CPP: simple cd", "cd /test/path"},
        {"CPP: cd with address", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo"},
        {"CPP: simple data", "data GNC"},
        {"CPP: simple data58", "data58 2bNcNLCKuxzGWJ5VP"},
        {"CPP: regPool empty", "regPool"},
        {"CPP: Token Pool Registration", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool"},
        {"CPP: Sacrificial Transaction", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000"},
    };

    std::cout << "const cppBytecodeTests = [" << std::endl;
    for (const auto& test : reverseTests) {
        std::vector<uint8_t> bytecodeWithHeader;
        if (compiler->compileWithHeader(test.second, bytecodeWithHeader)) {
            std::string base64Bytecode = tools->base64Encode(bytecodeWithHeader);
            std::cout << "    { name: \"" << test.first << "\", bytecode: \"" << base64Bytecode
                      << "\", expected: \"" << test.second << "\" }," << std::endl;
        } else {
            std::cout << "    // FAILED to compile: " << test.first << std::endl;
        }
    }
    std::cout << "];" << std::endl;

    // ════════════════════════════════════════════════════════════════════════════
    // SUMMARY
    // ════════════════════════════════════════════════════════════════════════════
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Summary: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (failed > 0) ? 1 : 0;
}
