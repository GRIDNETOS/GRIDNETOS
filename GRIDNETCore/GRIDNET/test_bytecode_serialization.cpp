// ════════════════════════════════════════════════════════════════════════════
// SUPPLEMENTARY BYTECODE FORMAT TEST (NOT AUTHORITATIVE)
// ════════════════════════════════════════════════════════════════════════════
//
// ⚠️  WARNING: This file uses ARTIFICIAL stub definitions, NOT the real
//     CScriptEngine. It is useful for quick bytecode format validation
//     without building the full GRIDNET solution.
//
// ⚠️  FOR PRODUCTION TESTING, use the authoritative tests in mainTests.cpp:
//     - CTests::testGridScriptCodeSerialization() - C++ standalone tests
//     - CTests::testJavaScriptBytecodeCrossValidation() - JS→C++ cross-validation
//     These tests use the REAL CScriptEngine with production codeword definitions.
//
// ⚠️  This file may have INDEX MISMATCHES with production if codeword definitions
//     change in CScriptEngine. Always verify with mainTests.cpp as the source of truth.
//
// For JavaScript tests, see: WebUI/test/test_bytecode_serialization.mjs
//
// Compile: cl /EHsc /std:c++17 test_bytecode_serialization.cpp /Fe:test_bytecode.exe
// ════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <regex>
#include <cstring>
#include <map>
#include <list>
#include <stack>

// ============================================================================
// Minimal Definition structure to mimic SE::Definition
// ============================================================================
struct Definition {
    std::string name;
    uint32_t id;
    bool extType;  // has inline parameters
    int inlineParamCount;
    bool hasBase58BinaryInLineParams;
    bool hasBase64BinaryInLineParams;

    bool isFindable() const { return true; }
};

// ============================================================================
// SHA256 implementation for checksums
// ============================================================================
class SHA256 {
public:
    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data) {
        uint32_t h[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        // Pre-processing: adding padding bits
        std::vector<uint8_t> msg = data;
        uint64_t originalLen = msg.size() * 8;
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) msg.push_back(0);
        for (int i = 7; i >= 0; i--) msg.push_back((originalLen >> (i * 8)) & 0xFF);

        // Process message in 64-byte chunks
        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; i++) {
                w[i] = (msg[chunk + i*4] << 24) | (msg[chunk + i*4+1] << 16) |
                       (msg[chunk + i*4+2] << 8) | msg[chunk + i*4+3];
            }
            for (int i = 16; i < 64; i++) {
                uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
                uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
                w[i] = w[i-16] + s0 + w[i-7] + s1;
            }

            uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
            uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

            for (int i = 0; i < 64; i++) {
                uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = hh + S1 + ch + k[i] + w[i];
                uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                hh = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }

            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        std::vector<uint8_t> result(32);
        for (int i = 0; i < 8; i++) {
            result[i*4] = (h[i] >> 24) & 0xFF;
            result[i*4+1] = (h[i] >> 16) & 0xFF;
            result[i*4+2] = (h[i] >> 8) & 0xFF;
            result[i*4+3] = h[i] & 0xFF;
        }
        return result;
    }

    static std::vector<uint8_t> doubleSha256(const std::vector<uint8_t>& data) {
        return hash(hash(data));
    }

private:
    static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
};

// ============================================================================
// Minimal Tools class for base64 encoding/decoding
// ============================================================================
class Tools {
public:
    static const char* BASE64_CHARS;

    static bool doStringsMatch(const std::string& a, const std::string& b) {
        return a == b;
    }

    static std::vector<uint8_t> stringToBytes(const std::string& str) {
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    static std::string bytesToString(const std::vector<uint8_t>& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    // Simple base64 encode (without checksum)
    static std::string base64Encode(const std::vector<uint8_t>& data) {
        std::string result;
        int val = 0, valb = -6;
        for (uint8_t c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) result.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
        while (result.size() % 4) result.push_back('=');
        return result;
    }

    // Simple base64 decode
    static bool base64Decode(const std::string& encoded, std::vector<uint8_t>& output) {
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[(unsigned char)BASE64_CHARS[i]] = i;

        output.clear();
        int val = 0, valb = -8;
        for (unsigned char c : encoded) {
            if (c == '=') break;
            if (T[c] == -1) return false;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                output.push_back((val >> valb) & 0xFF);
                valb -= 8;
            }
        }
        return true;
    }

    // Base64-check encode (with 4-byte double-SHA256 checksum)
    static std::string base64CheckEncode(const std::vector<uint8_t>& data) {
        // Calculate 4-byte checksum from double-SHA256
        std::vector<uint8_t> checksum = SHA256::doubleSha256(data);

        // Combine data + first 4 bytes of checksum
        std::vector<uint8_t> combined = data;
        combined.insert(combined.end(), checksum.begin(), checksum.begin() + 4);

        return base64Encode(combined);
    }

    // Base64-check decode (verify 4-byte checksum)
    static bool base64CheckDecode(const std::vector<uint8_t>& encoded, std::vector<uint8_t>& output) {
        std::string encodedStr(encoded.begin(), encoded.end());
        std::vector<uint8_t> decoded;
        if (!base64Decode(encodedStr, decoded)) return false;
        if (decoded.size() < 4) return false;  // Minimum 4 bytes for checksum (empty payload)

        // Extract payload and checksum
        output.assign(decoded.begin(), decoded.end() - 4);
        std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());

        // Verify checksum
        std::vector<uint8_t> expectedChecksum = SHA256::doubleSha256(output);
        for (int i = 0; i < 4; i++) {
            if (checksum[i] != expectedChecksum[i]) return false;
        }
        return true;
    }

    // Base58 alphabet (Bitcoin-style)
    static const char* BASE58_CHARS;

    // Base58 encode (without checksum)
    static std::string base58Encode(const std::vector<uint8_t>& data) {
        if (data.empty()) return "";

        // Count leading zeros
        size_t zeroes = 0;
        for (auto b : data) {
            if (b == 0) zeroes++;
            else break;
        }

        // Convert to base58
        std::vector<uint8_t> b58((data.size() - zeroes) * 138 / 100 + 1);
        size_t length = 0;
        for (size_t i = zeroes; i < data.size(); i++) {
            int carry = data[i];
            size_t j = 0;
            for (auto it = b58.rbegin(); (carry != 0 || j < length) && it != b58.rend(); ++it, ++j) {
                carry += 256 * (*it);
                *it = carry % 58;
                carry /= 58;
            }
            length = j;
        }

        auto it = b58.begin() + (b58.size() - length);
        std::string result(zeroes, '1');
        while (it != b58.end()) {
            result += BASE58_CHARS[*it++];
        }
        return result;
    }

    // Base58Check encode (with 4-byte single-SHA256 checksum)
    // NOTE: Uses SINGLE SHA256, matching reference CTools implementation in lib/tools.js
    static std::string base58CheckEncode(const std::vector<uint8_t>& data) {
        // Calculate 4-byte checksum from single SHA256 (NOT double)
        std::vector<uint8_t> checksum = SHA256::hash(data);

        // Combine data + first 4 bytes of checksum
        std::vector<uint8_t> combined = data;
        combined.insert(combined.end(), checksum.begin(), checksum.begin() + 4);

        return base58Encode(combined);
    }

    // Base58 decode (without checksum verification)
    static bool base58Decode(const std::string& encoded, std::vector<uint8_t>& output) {
        if (encoded.empty()) return false;

        // Count leading '1's
        size_t zeroes = 0;
        for (auto c : encoded) {
            if (c == '1') zeroes++;
            else break;
        }

        // Decode
        std::vector<uint8_t> b256(encoded.size() * 733 / 1000 + 1);
        size_t length = 0;
        for (size_t i = zeroes; i < encoded.size(); i++) {
            const char* p = strchr(BASE58_CHARS, encoded[i]);
            if (!p) return false;
            int carry = p - BASE58_CHARS;
            size_t j = 0;
            for (auto it = b256.rbegin(); (carry != 0 || j < length) && it != b256.rend(); ++it, ++j) {
                carry += 58 * (*it);
                *it = carry % 256;
                carry /= 256;
            }
            length = j;
        }

        output.clear();
        output.resize(zeroes, 0);
        auto it = b256.begin() + (b256.size() - length);
        while (it != b256.end() && *it == 0) ++it;
        while (it != b256.end()) output.push_back(*it++);

        return true;
    }

    // Base58Check decode (verify 4-byte checksum)
    // NOTE: Uses SINGLE SHA256, matching reference CTools implementation in Tools.cpp
    static bool base58CheckDecode(const std::string& encoded, std::vector<uint8_t>& output) {
        std::vector<uint8_t> decoded;
        if (!base58Decode(encoded, decoded)) return false;
        if (decoded.size() < 4) return false;  // Minimum 4 bytes for checksum (empty payload)

        // Extract payload and checksum
        output.assign(decoded.begin(), decoded.end() - 4);
        std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());

        // Verify checksum - SINGLE SHA256 (NOT double)
        std::vector<uint8_t> expectedChecksum = SHA256::hash(output);
        for (int i = 0; i < 4; i++) {
            if (checksum[i] != expectedChecksum[i]) return false;
        }
        return true;
    }
};

const char* Tools::BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char* Tools::BASE58_CHARS = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// ============================================================================
// Simplified GridScriptCompiler for testing
// ============================================================================
class GridScriptCompiler {
public:
    int mBytecodeVersion = 2;  // V2 bytecode
    std::vector<uint8_t> mCompiledByteCode;
    std::vector<uint8_t> mCurrentOpCode;
    bool mCompilingExtendedID = false;
    size_t mCurrentOffset = 0;
    Tools mTools;

    // V2 keyword image hash chain
    std::vector<uint8_t> mKeywordImageHash;
    bool mHashInitialized = false;

    // Keyword definitions
    std::list<Definition> mDefinitions;

    GridScriptCompiler() {
        initDefinitions();
        initKeywordHash();
    }

    // Initialize V2 keyword hash chain with seed
    void initKeywordHash() {
        // H₀ = SHA256("GRIDSCRIPT_V2_KEYWORD_IMAGE")
        std::string seed = "GRIDSCRIPT_V2_KEYWORD_IMAGE";
        std::vector<uint8_t> seedBytes(seed.begin(), seed.end());
        mKeywordImageHash = SHA256::hash(seedBytes);
        mHashInitialized = true;
    }

    // Reset hash to initial state (for fresh compilation)
    void resetKeywordHash() {
        initKeywordHash();
    }

    // Update hash chain with keyword name
    // Hₙ = SHA256(Hₙ₋₁ || keyword_name)
    void updateKeywordHash(const std::string& keywordName) {
        if (!mHashInitialized) initKeywordHash();

        // Concatenate current hash with keyword name
        std::vector<uint8_t> combined = mKeywordImageHash;
        combined.insert(combined.end(), keywordName.begin(), keywordName.end());

        // Update hash
        mKeywordImageHash = SHA256::hash(combined);
    }

    void initDefinitions() {
        // IMPORTANT: Opcode IDs are calculated dynamically as BASE_OPCODE_ID (9) + array_index
        // This MUST match the JavaScript production compiler (lib/GridScriptCompiler.js)
        // and the C++ production implementation for cross-validation to work.
        //
        // Array format: [name, allowedInKernelMode, inlineParams, hasBase58BinaryInLineParams, hasBase64BinaryInLineParams]
        // Only keywords needed for testing are included, but at their CORRECT indices

        const int BASE_OPCODE_ID = 9;

        // We store codewords at their production indices to get correct opcode IDs
        // Production array has 293 entries (290 + 3 memory allocation functions)
        // Memory functions (free, memclr, alloc) at indices 78-80
        // File access functions DISABLED (GRIDSCRIPT_DISABLE_FILE_ACCESS=1)
        // All indices >= 78 are shifted by +3
        // Syntax: {index, name, extType, inlineParams, hasBase58, hasBase64}
        struct CodewordDef {
            int index;
            std::string name;
            bool extType;
            int inlineParams;
            bool hasBase58;
            bool hasBase64;
        };

        // IMPORTANT: Use ACTUAL indices from JavaScript production compiler runtime values
        // JavaScript includes memory functions (free, memclr, alloc) at indices 78-80
        // The comments in GridScriptCompiler.js are OUTDATED - use actual runtime values
        // Items before memory functions (index < 78): actual_index = index
        // Items after memory functions (index >= 78): actual_index = original_index + 3
        std::vector<CodewordDef> codewords = {
            // index 0: data64 - opcode 9 (before memory functions)
            {0, "data64", true, 1, false, true},
            // index 2: data - opcode 11 (before memory functions)
            {2, "data", true, 1, false, false},
            // index 97: getVarEx - opcode 106 (94+3 due to memory functions)
            {97, "getVarEx", false, 0, false, false},
            // index 100: setVarEx - opcode 109 (97+3 due to memory functions)
            {100, "setVarEx", false, 0, false, false},
            // index 119: xvalue - opcode 128 (116+3 due to memory functions)
            {119, "xvalue", false, 0, false, false},
            // index 136: data58 - opcode 145 (133+3 due to memory functions)
            {136, "data58", true, 1, true, false},
            // index 151: echo - opcode 160 (148+3 due to memory functions)
            {151, "echo", false, 0, false, false},
            // index 156: cd - opcode 165 (153+3 due to memory functions)
            {156, "cd", true, 1, false, false},
            // index 197: sacrifice - opcode 206 (194+3 due to memory functions)
            {197, "sacrifice", true, 1, false, false},
            // index 215: regPoolEx - opcode 224 (212+3 due to memory functions)
            {215, "regPoolEx", false, 0, false, false},
            // index 292: regPool - opcode 301 (already at correct position in JS)
            {292, "regPool", true, 1, false, true},
        };

        for (const auto& cw : codewords) {
            uint32_t opcodeId = BASE_OPCODE_ID + cw.index;
            mDefinitions.push_back({cw.name, opcodeId, cw.extType, cw.inlineParams, cw.hasBase58, cw.hasBase64});
            std::cout << "    Registered: " << cw.name << " -> opcode " << opcodeId << std::endl;
        }
    }

    Definition* findDefinition(const std::string& name) {
        for (auto& def : mDefinitions) {
            if (def.name == name) return &def;
        }
        return nullptr;
    }

    bool setIDBits(uint32_t id) {
        if (id > 127) {
            mCurrentOpCode.resize(2);
            mCurrentOpCode[0] = (id >> 8) | 0x80;
            mCurrentOpCode[1] = id & 0xFF;
            mCompilingExtendedID = true;
        } else {
            mCurrentOpCode.resize(1);
            mCurrentOpCode[0] = id & 0xFF;
            mCompilingExtendedID = false;
        }
        return true;
    }

    bool setContentBits(const std::vector<uint8_t>& content) {
        size_t len = content.size();
        if (len <= 127) {
            mCurrentOpCode.push_back(len);
        } else {
            // Extended length encoding
            int numBytes = 0;
            size_t temp = len;
            while (temp > 0) { numBytes++; temp >>= 8; }
            mCurrentOpCode.push_back(numBytes | 0x80);
            for (int i = 0; i < numBytes; i++) {
                mCurrentOpCode.push_back((len >> (i * 8)) & 0xFF);
            }
        }
        mCurrentOpCode.insert(mCurrentOpCode.end(), content.begin(), content.end());
        return true;
    }

    // Compile with V2 header (version byte + keyword image hash)
    bool compileWithHeader(const std::string& sourceCode, std::vector<uint8_t>& result) {
        std::vector<uint8_t> opcodes;
        if (!compile(sourceCode, opcodes)) {
            return false;
        }

        // Build V2 header: [VERSION_BYTE (192+version)][32-byte keyword hash]
        result.clear();
        result.push_back(192 + mBytecodeVersion);  // 194 for V2
        result.insert(result.end(), mKeywordImageHash.begin(), mKeywordImageHash.end());
        result.insert(result.end(), opcodes.begin(), opcodes.end());

        std::cout << "  [COMPILE] V2 bytecode with header: " << result.size() << " bytes" << std::endl;
        return true;
    }

    bool compile(const std::string& sourceCode, std::vector<uint8_t>& result) {
        std::cout << "  [COMPILE] Input: \"" << sourceCode.substr(0, 80) << (sourceCode.size() > 80 ? "..." : "") << "\"" << std::endl;

        // Reset keyword hash for fresh compilation
        resetKeywordHash();

        std::regex e("\\S+");
        std::regex flagR("^[\\-\\+][a-zA-Z0-9]+$");

        std::string code = sourceCode;
        std::smatch m;
        std::vector<uint8_t> byteCode;

        Definition* activeDefinition = nullptr;
        bool waitingForParam = false;
        bool waitingForFlagValue = false;
        std::string pendingFlag;
        uint32_t paramsTraversed = 0;
        uint32_t requiredParams = 0;
        std::vector<uint8_t> paremetersVec;

        while (std::regex_search(code, m, e)) {
            std::string token = m.str();
            bool commandReady = false;

            if (!waitingForParam) {
                // Look for command
                activeDefinition = findDefinition(token);

                if (activeDefinition) {
                    std::cout << "    Found command: " << activeDefinition->name
                              << " (id=" << activeDefinition->id
                              << ", extType=" << activeDefinition->extType
                              << ", inlineParams=" << activeDefinition->inlineParamCount << ")" << std::endl;

                    // V2: Update keyword hash chain with keyword name (canonical case)
                    // IMPORTANT: Only update hash for opcodes > BYTECODE_ID_STRING_LITERAL (122)
                    // This matches JavaScript: codeword.opcode > this.BYTECODE_ID_STRING_LITERAL
                    // where BYTECODE_ID_STRING_LITERAL = 122 in lib/GridScriptCompiler.js
                    const uint32_t BYTECODE_ID_STRING_LITERAL = 122;
                    if (mBytecodeVersion >= 2 && activeDefinition->id > BYTECODE_ID_STRING_LITERAL) {
                        updateKeywordHash(activeDefinition->name);
                    }

                    if (activeDefinition->extType && activeDefinition->inlineParamCount > 0) {
                        setIDBits(activeDefinition->id);
                        waitingForParam = true;
                        requiredParams = activeDefinition->inlineParamCount;
                        paramsTraversed = 0;
                        paremetersVec.clear();
                        std::cout << "    Waiting for " << requiredParams << " inline param(s)" << std::endl;
                    } else {
                        setIDBits(activeDefinition->id);
                        commandReady = true;
                    }
                } else {
                    // Unknown token - treat as user-defined or literal
                    std::cout << "    Unknown token: " << token << std::endl;
                    // For simplicity, skip unknown tokens in this test
                }
            } else {
                // Waiting for inline parameter
                std::string currentToken = token;

                // V2+ Flag Support: Check if token is a flag pattern
                if (mBytecodeVersion >= 2 && !waitingForFlagValue && std::regex_match(currentToken, flagR)) {
                    std::cout << "    Detected flag: " << currentToken << std::endl;
                    pendingFlag = currentToken;
                    waitingForFlagValue = true;
                    code = m.suffix().str();
                    continue;
                }

                std::string param;
                if (waitingForFlagValue) {
                    param = pendingFlag + " " + currentToken;
                    std::cout << "    Combined flag+value: " << param.substr(0, 60) << (param.size() > 60 ? "..." : "") << std::endl;
                    pendingFlag.clear();
                    waitingForFlagValue = false;
                    paramsTraversed++;
                } else {
                    paramsTraversed++;
                    param = currentToken;
                    std::cout << "    Inline param " << paramsTraversed << ": " << param.substr(0, 60) << (param.size() > 60 ? "..." : "") << std::endl;
                }

                // Handle flag prefix for base58/base64 params
                bool hasFlag = (param.size() > 2 && (param[0] == '-' || param[0] == '+') && param.find(' ') != std::string::npos);
                std::string flagPrefix;
                std::string valueToProcess = param;

                if (hasFlag && mBytecodeVersion >= 2) {
                    size_t spacePos = param.find(' ');
                    flagPrefix = param.substr(0, spacePos + 1);
                    valueToProcess = param.substr(spacePos + 1);
                    std::cout << "    Extracted flag prefix: \"" << flagPrefix << "\"" << std::endl;
                }

                if (activeDefinition->hasBase58BinaryInLineParams) {
                    std::vector<uint8_t> decodedData;
                    if (!mTools.base58CheckDecode(valueToProcess, decodedData)) {
                        std::cout << "    ERROR: Failed to decode base58" << std::endl;
                        return false;
                    }
                    if (hasFlag && mBytecodeVersion >= 2) {
                        paremetersVec.insert(paremetersVec.end(), flagPrefix.begin(), flagPrefix.end());
                    }
                    paremetersVec.insert(paremetersVec.end(), decodedData.begin(), decodedData.end());
                    std::cout << "    Base58 decoded, total param bytes: " << paremetersVec.size() << std::endl;
                } else if (activeDefinition->hasBase64BinaryInLineParams) {
                    std::vector<uint8_t> decodedData;
                    auto valueBytes = mTools.stringToBytes(valueToProcess);
                    if (!mTools.base64CheckDecode(valueBytes, decodedData)) {
                        std::cout << "    ERROR: Failed to decode base64: " << valueToProcess.substr(0, 40) << std::endl;
                        return false;
                    }
                    if (hasFlag && mBytecodeVersion >= 2) {
                        paremetersVec.insert(paremetersVec.end(), flagPrefix.begin(), flagPrefix.end());
                    }
                    paremetersVec.insert(paremetersVec.end(), decodedData.begin(), decodedData.end());
                    std::cout << "    Base64 decoded, total param bytes: " << paremetersVec.size() << std::endl;
                } else {
                    paremetersVec.insert(paremetersVec.end(), param.begin(), param.end());
                }

                if (paramsTraversed >= requiredParams) {
                    setContentBits(paremetersVec);
                    commandReady = true;
                    std::cout << "    All params collected, opcode size: " << mCurrentOpCode.size() << std::endl;
                }
            }

            if (commandReady) {
                byteCode.insert(byteCode.end(), mCurrentOpCode.begin(), mCurrentOpCode.end());
                activeDefinition = nullptr;
                mCurrentOpCode.clear();
                paremetersVec.clear();
                waitingForParam = false;
                paramsTraversed = 0;
                requiredParams = 0;
            }

            code = m.suffix().str();
        }

        // Handle missing inline params for V2+
        if (waitingForParam && mBytecodeVersion >= 2) {
            setContentBits(paremetersVec);
            byteCode.insert(byteCode.end(), mCurrentOpCode.begin(), mCurrentOpCode.end());
        }

        result = byteCode;
        std::cout << "  [COMPILE] Output bytecode size: " << result.size() << " bytes" << std::endl;
        return true;
    }

    bool decompile(const std::vector<uint8_t>& bytecode, std::string& sourceCode) {
        std::cout << "  [DECOMPILE] Input bytecode size: " << bytecode.size() << " bytes" << std::endl;

        std::vector<std::string> codeWords;
        size_t offset = 0;

        // Detect V2 bytecode format: first byte >= 192 means V2+ with header
        // V2 header: [VERSION_BYTE (192+version)][32-byte keyword image hash]
        if (bytecode.size() > 0 && bytecode[0] >= 192) {
            int detectedVersion = bytecode[0] - 192;
            std::cout << "    Detected V2 bytecode (version=" << detectedVersion << ")" << std::endl;

            if (bytecode.size() < 33) {
                std::cout << "    Error: V2 bytecode too short (need at least 33 bytes for header)" << std::endl;
                return false;
            }

            // Skip version byte (1) + hash (32) = 33 bytes
            offset = 33;
            std::cout << "    Skipping V2 header, opcodes start at offset " << offset << std::endl;

            // For V2, we should verify the hash, but for cross-validation we just skip it
            // since the JavaScript compiler has already computed the correct hash
        }

        while (offset < bytecode.size()) {
            // Read opcode ID
            uint32_t id;
            if (bytecode[offset] & 0x80) {
                // Extended ID (2 bytes)
                if (offset + 1 >= bytecode.size()) break;
                id = ((bytecode[offset] & 0x7F) << 8) | bytecode[offset + 1];
                offset += 2;
            } else {
                id = bytecode[offset];
                offset++;
            }

            // Find definition by ID
            Definition* def = nullptr;
            for (auto& d : mDefinitions) {
                if (d.id == id) { def = &d; break; }
            }

            if (!def) {
                std::cout << "    Unknown opcode ID: " << id << " at offset " << (offset - 1) << std::endl;
                break;
            }

            std::cout << "    Opcode: " << def->name << " (id=" << id << ", extType=" << def->extType << ")" << std::endl;

            if (def->extType) {
                // Read content length
                if (offset >= bytecode.size()) break;
                size_t contentLen;
                if (bytecode[offset] & 0x80) {
                    int numBytes = bytecode[offset] & 0x7F;
                    offset++;
                    contentLen = 0;
                    for (int i = 0; i < numBytes && offset < bytecode.size(); i++) {
                        contentLen |= (size_t)bytecode[offset++] << (i * 8);
                    }
                } else {
                    contentLen = bytecode[offset++];
                }

                std::cout << "    Content length: " << contentLen << std::endl;

                // Read content
                if (offset + contentLen > bytecode.size()) break;
                std::vector<uint8_t> content(bytecode.begin() + offset, bytecode.begin() + offset + contentLen);
                offset += contentLen;

                codeWords.push_back(def->name);

                if (contentLen == 0) {
                    // Empty inline param for V2+
                    codeWords.push_back("");
                } else if (def->hasBase58BinaryInLineParams || def->hasBase64BinaryInLineParams) {
                    // Check for flag prefix
                    std::string flagPrefix;
                    std::vector<uint8_t> binaryContent = content;

                    if (mBytecodeVersion >= 2 && content.size() > 3 && (content[0] == '-' || content[0] == '+')) {
                        size_t spacePos = 0;
                        for (size_t i = 1; i < content.size() && i < 64; i++) {
                            if (content[i] == ' ') {
                                bool validFlag = true;
                                for (size_t j = 1; j < i; j++) {
                                    char c = content[j];
                                    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                                        validFlag = false;
                                        break;
                                    }
                                }
                                if (validFlag) {
                                    spacePos = i;
                                    break;
                                }
                            }
                        }

                        if (spacePos > 0) {
                            flagPrefix = std::string(content.begin(), content.begin() + spacePos + 1);
                            binaryContent = std::vector<uint8_t>(content.begin() + spacePos + 1, content.end());
                            std::cout << "    Detected flag prefix: \"" << flagPrefix << "\"" << std::endl;
                        }
                    }

                    std::string encoded;
                    if (def->hasBase58BinaryInLineParams) {
                        encoded = mTools.base58CheckEncode(binaryContent);
                    } else {
                        encoded = mTools.base64CheckEncode(binaryContent);
                    }
                    codeWords.push_back(flagPrefix + encoded);
                    std::cout << "    Encoded param: " << (flagPrefix + encoded).substr(0, 60) << "..." << std::endl;
                } else {
                    // Plain string
                    std::string param(content.begin(), content.end());
                    codeWords.push_back(param);
                }
            } else {
                codeWords.push_back(def->name);
            }
        }

        // Join non-empty codewords
        sourceCode.clear();
        for (const auto& word : codeWords) {
            if (word.empty()) continue;
            if (!sourceCode.empty()) sourceCode += " ";
            sourceCode += word;
        }

        std::cout << "  [DECOMPILE] Output: \"" << sourceCode.substr(0, 80) << (sourceCode.size() > 80 ? "..." : "") << "\"" << std::endl;
        return true;
    }
};

// ============================================================================
// Test harness
// ============================================================================
struct TestCase {
    std::string name;
    std::string input;
    bool shouldCompile;
    bool shouldRoundTrip;  // Should input == decompiled output
};

// ============================================================================
// Base64/Base64Check Validation Tests
// These MUST pass before any other tests can run
// ============================================================================
struct Base64ValidationTest {
    std::string name;
    std::vector<uint8_t> input;
    std::string expectedBase64;
};

bool runBase64ValidationTests() {
    std::cout << "========================================" << std::endl;
    std::cout << "Base64/Base64Check Encoding Validation" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Hardcoded test vectors for base64 encoding validation
    // These are industry-standard test vectors
    std::vector<Base64ValidationTest> base64Tests = {
        // RFC 4648 test vectors
        { "Empty string", {}, "" },
        { "Single byte 'f'", {'f'}, "Zg==" },
        { "Two bytes 'fo'", {'f', 'o'}, "Zm8=" },
        { "Three bytes 'foo'", {'f', 'o', 'o'}, "Zm9v" },
        { "Four bytes 'foob'", {'f', 'o', 'o', 'b'}, "Zm9vYg==" },
        { "Five bytes 'fooba'", {'f', 'o', 'o', 'b', 'a'}, "Zm9vYmE=" },
        { "Six bytes 'foobar'", {'f', 'o', 'o', 'b', 'a', 'r'}, "Zm9vYmFy" },

        // Standard test strings
        { "Hello World", {'H','e','l','l','o',' ','W','o','r','l','d'}, "SGVsbG8gV29ybGQ=" },
        { "Base64 test", {'B','a','s','e','6','4',' ','t','e','s','t'}, "QmFzZTY0IHRlc3Q=" },

        // Binary data test (all byte values 0-15)
        { "Binary 0x00-0x0F", {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F}, "AAECAwQFBgcICQoLDA0ODw==" },

        // High byte values
        { "Binary 0xF0-0xFF", {0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}, "8PHy8/T19vf4+fr7/P3+/w==" },
    };

    int validationPassed = 0;
    int validationFailed = 0;

    std::cout << "--- Base64 Encode Tests ---" << std::endl;
    for (const auto& test : base64Tests) {
        std::string result = Tools::base64Encode(test.input);
        bool passed = (result == test.expectedBase64);

        if (passed) {
            std::cout << "  [PASS] " << test.name << std::endl;
            validationPassed++;
        } else {
            std::cout << "  [FAIL] " << test.name << std::endl;
            std::cout << "         Expected: \"" << test.expectedBase64 << "\"" << std::endl;
            std::cout << "         Got:      \"" << result << "\"" << std::endl;
            validationFailed++;
        }
    }

    std::cout << std::endl;
    std::cout << "--- Base64 Decode Tests ---" << std::endl;
    for (const auto& test : base64Tests) {
        if (test.expectedBase64.empty() && test.input.empty()) {
            // Skip empty test for decode
            std::cout << "  [SKIP] " << test.name << " (empty)" << std::endl;
            continue;
        }

        std::vector<uint8_t> decoded;
        bool decodeSuccess = Tools::base64Decode(test.expectedBase64, decoded);
        bool passed = decodeSuccess && (decoded == test.input);

        if (passed) {
            std::cout << "  [PASS] " << test.name << std::endl;
            validationPassed++;
        } else {
            std::cout << "  [FAIL] " << test.name << std::endl;
            if (!decodeSuccess) {
                std::cout << "         Decode function returned false" << std::endl;
            } else {
                std::cout << "         Expected " << test.input.size() << " bytes, got " << decoded.size() << " bytes" << std::endl;
                if (!decoded.empty() && !test.input.empty()) {
                    std::cout << "         First byte expected: 0x" << std::hex << (int)test.input[0]
                              << " got: 0x" << (int)decoded[0] << std::dec << std::endl;
                }
            }
            validationFailed++;
        }
    }

    std::cout << std::endl;
    std::cout << "--- Base64 Round-Trip Tests ---" << std::endl;

    // Additional round-trip tests with various data patterns
    std::vector<std::pair<std::string, std::vector<uint8_t>>> roundTripTests = {
        { "All zeros (16 bytes)", std::vector<uint8_t>(16, 0x00) },
        { "All 0xFF (16 bytes)", std::vector<uint8_t>(16, 0xFF) },
        { "Alternating 0x55/0xAA", {0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA} },
        { "Sequential 0-255", {} }, // Will be filled below
        { "Padding test (1 byte)", {0x42} },
        { "Padding test (2 bytes)", {0x42, 0x43} },
        { "Padding test (3 bytes)", {0x42, 0x43, 0x44} },
    };

    // Fill sequential test
    for (auto& test : roundTripTests) {
        if (test.first == "Sequential 0-255") {
            test.second.resize(256);
            for (int i = 0; i < 256; i++) test.second[i] = (uint8_t)i;
        }
    }

    for (const auto& test : roundTripTests) {
        std::string encoded = Tools::base64Encode(test.second);
        std::vector<uint8_t> decoded;
        bool decodeSuccess = Tools::base64Decode(encoded, decoded);
        bool passed = decodeSuccess && (decoded == test.second);

        if (passed) {
            std::cout << "  [PASS] " << test.first << " (encoded to " << encoded.size() << " chars)" << std::endl;
            validationPassed++;
        } else {
            std::cout << "  [FAIL] " << test.first << std::endl;
            std::cout << "         Encoded: \"" << encoded.substr(0, 50) << (encoded.size() > 50 ? "..." : "") << "\"" << std::endl;
            if (!decodeSuccess) {
                std::cout << "         Decode failed" << std::endl;
            } else {
                std::cout << "         Size mismatch: expected " << test.second.size() << " got " << decoded.size() << std::endl;
            }
            validationFailed++;
        }
    }

    std::cout << std::endl;
    std::cout << "--- Base64Check Encode/Decode Tests ---" << std::endl;

    // Test base64Check (encode then decode should match)
    std::vector<std::pair<std::string, std::vector<uint8_t>>> base64CheckTests = {
        { "Simple string", {'T','e','s','t'} },
        { "Binary data", {0x00, 0x01, 0x02, 0xFD, 0xFE, 0xFF} },
        { "Empty data", {} },
    };

    for (const auto& test : base64CheckTests) {
        std::string encoded = Tools::base64CheckEncode(test.second);
        std::vector<uint8_t> encodedBytes(encoded.begin(), encoded.end());
        std::vector<uint8_t> decoded;
        bool decodeSuccess = Tools::base64CheckDecode(encodedBytes, decoded);
        bool passed = decodeSuccess && (decoded == test.second);

        if (passed) {
            std::cout << "  [PASS] " << test.first << std::endl;
            validationPassed++;
        } else {
            std::cout << "  [FAIL] " << test.first << std::endl;
            std::cout << "         Encoded: \"" << encoded << "\"" << std::endl;
            if (!decodeSuccess) {
                std::cout << "         Decode failed" << std::endl;
            } else {
                std::cout << "         Data mismatch after round-trip" << std::endl;
            }
            validationFailed++;
        }
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Base64 Validation Summary: " << validationPassed << " passed, " << validationFailed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    if (validationFailed > 0) {
        std::cerr << "FATAL: Base64/Base64Check encoding validation FAILED!" << std::endl;
        std::cerr << "       The encoding implementation is not working correctly." << std::endl;
        std::cerr << "       Cannot proceed with bytecode serialization tests." << std::endl;
        std::cerr << std::endl;
        std::cerr << "       Please verify:" << std::endl;
        std::cerr << "       1. Base64 character set is correct" << std::endl;
        std::cerr << "       2. Padding (=) is handled correctly" << std::endl;
        std::cerr << "       3. Binary data is encoded/decoded without corruption" << std::endl;
        return false;
    }

    std::cout << "Base64/Base64Check validation PASSED - proceeding with tests" << std::endl;
    std::cout << std::endl;
    return true;
}

int main() {
    // ══════════════════════════════════════════════════════════════════════════
    // OBLIGATORY: Run base64 validation tests FIRST
    // If these fail, the bytecode serialization tests cannot be trusted
    // ══════════════════════════════════════════════════════════════════════════
    if (!runBase64ValidationTests()) {
        std::cerr << std::endl;
        std::cerr << "╔════════════════════════════════════════════════════════════╗" << std::endl;
        std::cerr << "║  ABORTING: Base64 encoding validation failed!              ║" << std::endl;
        std::cerr << "║  Fix the Base64 implementation before running other tests. ║" << std::endl;
        std::cerr << "╚════════════════════════════════════════════════════════════╝" << std::endl;
        return 2; // Special exit code for validation failure
    }

    GridScriptCompiler compiler;

    std::vector<TestCase> tests = {
        // ══════════════════════════════════════════════════════════════════
        // TESTS FROM CTests::testGridScriptCodeSerialization()
        // ══════════════════════════════════════════════════════════════════

        // s4: Multiple data58 values with addresses
        // "TestData" -> base58Check = "2bNcNLCKuxzGWJ5VP"
        // "This is a larger test payload for data58 command" -> base58Check = "MG97WaBEMjd3YwtxH7SMKZrLYewZzN3t9FfCvnZxEc3UBdvQWhgBUSCKpLton3QD6sHbJ2A"
        {
            "s4 - data58 with multiple addresses",
            "data58 2bNcNLCKuxzGWJ5VP data 16RcPSCdd8has7pTindEXt3XsoQeppBM5q data 1PeTP9d9hNNux3w2yUxBiPviNBahNdxYxT data GNC data58 MG97WaBEMjd3YwtxH7SMKZrLYewZzN3t9FfCvnZxEc3UBdvQWhgBUSCKpLton3QD6sHbJ2A xvalue",
            true,
            true
        },

        // s9: cd with data58 and regPoolEx (0 inline params)
        // "This is test data for s9-like test case with cd and regPoolEx" -> base58Check
        {
            "s9 - cd with data58 and regPoolEx",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 8T2EsZVLVHo3s91PCidKMNQAq1YLJy7apySsSVbTxoXpLJUoKttSwDe24Tw14oiTsbhF6Qd6xEDXvsLaXDXHXkWyU regPoolEx",
            true,
            true
        },

        // s15: regPool WITHOUT inline argument (V2+ allows omitting inline args)
        // Same data58 as s9, but with regPool (which allows omitting inline param)
        {
            "s15 - regPool without inline param (data from stack)",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 8T2EsZVLVHo3s91PCidKMNQAq1YLJy7apySsSVbTxoXpLJUoKttSwDe24Tw14oiTsbhF6Qd6xEDXvsLaXDXHXkWyU regPool",
            true,
            true
        },

        // s16: regPool WITH inline argument (-t flag syntax)
        // "Test payload with flag" -> base64Check = "VGVzdCBwYXlsb2FkIHdpdGggZmxhZ5qOpkk="
        {
            "s16 - regPool with -t flag (base64 data)",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / regPool -t VGVzdCBwYXlsb2FkIHdpdGggZmxhZ5qOpkk=",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // BASIC DATA TESTS
        // ══════════════════════════════════════════════════════════════════

        // Simple data58 encoding (base58Check with checksum)
        // "TestData" -> base58Check = "2bNcNLCKuxzGWJ5VP"
        {
            "simple data58",
            "data58 2bNcNLCKuxzGWJ5VP",
            true,
            true
        },

        // Simple cd command
        {
            "simple cd",
            "cd /test/path",
            true,
            true
        },

        // cd with path
        {
            "cd with address",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo",
            true,
            true
        },

        // data command
        {
            "simple data",
            "data GNC",
            true,
            true
        },

        // Multiple data commands
        {
            "multiple data",
            "data GNC data 15ZtqfUW5wjv23SK7J9F3kbhC2nxLdsnKV data 0",
            true,
            true
        },

        // xvalue command
        {
            "xvalue command",
            "data GNC xvalue",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // FLAG TESTS - Minus flag (-flag)
        // All values are base64Check encoded (with 4-byte double-SHA256 checksum)
        // ══════════════════════════════════════════════════════════════════

        // Test -t flag (single char)
        // "Hello World" -> base64Check = "SGVsbG8gV29ybGRCqHOs"
        {
            "-t flag (single char)",
            "regPool -t SGVsbG8gV29ybGRCqHOs",
            true,
            true
        },

        // Test -help flag (multi char)
        // "Help" -> base64Check = "SGVscHND5n4="
        {
            "-help flag (multi char)",
            "regPool -help SGVscHND5n4=",
            true,
            true
        },

        // Test -page123 flag (with numbers)
        // "Page" -> base64Check = "UGFnZQ4g+mo="
        {
            "-page123 flag (with numbers)",
            "regPool -page123 UGFnZQ4g+mo=",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // FLAG TESTS - Plus flag (+flag)
        // All values are base64Check encoded (with 4-byte double-SHA256 checksum)
        // ══════════════════════════════════════════════════════════════════

        // Test +t flag (single char)
        // "Hello World" -> base64Check = "SGVsbG8gV29ybGRCqHOs"
        {
            "+t flag (single char)",
            "regPool +t SGVsbG8gV29ybGRCqHOs",
            true,
            true
        },

        // Test +verbose flag (multi char)
        // "Verbose" -> base64Check = "VmVyYm9zZU1032E="
        {
            "+verbose flag (multi char)",
            "regPool +verbose VmVyYm9zZU1032E=",
            true,
            true
        },

        // Test +opt42 flag (with numbers)
        // "Option" -> base64Check = "T3B0aW9u3OJhLg=="
        {
            "+opt42 flag (with numbers)",
            "regPool +opt42 T3B0aW9u3OJhLg==",
            true,
            true
        },

        // s16 variant with +t flag (plus flag version)
        // "Test payload with flag" -> base64Check = "VGVzdCBwYXlsb2FkIHdpdGggZmxhZ5qOpkk="
        {
            "s16 variant - regPool with +t flag",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / regPool +t VGVzdCBwYXlsb2FkIHdpdGggZmxhZ5qOpkk=",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // LONG FLAG TESTS
        // All values are base64Check encoded (with 4-byte double-SHA256 checksum)
        // ══════════════════════════════════════════════════════════════════

        // Long minus flag
        // "Long" -> base64Check = "TG9uZ6fWI/0="
        {
            "long minus flag -verylongflagname",
            "regPool -verylongflagname TG9uZ6fWI/0=",
            true,
            true
        },

        // Long plus flag
        // "Long" -> base64Check = "TG9uZ6fWI/0="
        {
            "long plus flag +verylongflagname",
            "regPool +verylongflagname TG9uZ6fWI/0=",
            true,
            true
        },

        // Flag with many numbers
        // "Numbers" -> base64Check = "TnVtYmVyczGtoUw="
        {
            "flag with numbers -opt1234567890",
            "regPool -opt1234567890 TnVtYmVyczGtoUw=",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // EDGE CASES
        // ══════════════════════════════════════════════════════════════════

        // Empty inline param (V2+ allows this)
        {
            "empty inline param",
            "regPool",
            true,
            true
        },

        // Multiple cd commands
        {
            "multiple cd commands",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd /",
            true,
            true
        },

        // cd with special path
        {
            "cd with / path",
            "cd /",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // DATA64 TESTS
        // All values are base64Check encoded (with 4-byte double-SHA256 checksum)
        // ══════════════════════════════════════════════════════════════════

        // Simple data64
        // "Hello World" -> base64Check = "SGVsbG8gV29ybGRCqHOs"
        {
            "data64 simple",
            "data64 SGVsbG8gV29ybGRCqHOs",
            true,
            true
        },

        // data64 longer content
        // "This is a longer test string for base64 encoding" -> base64Check
        {
            "data64 longer content",
            "data64 VGhpcyBpcyBhIGxvbmdlciB0ZXN0IHN0cmluZyBmb3IgYmFzZTY0IGVuY29kaW5nhCrGaw==",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // COMBINED COMMAND SEQUENCES
        // data58 values are base58Check encoded (with 4-byte double-SHA256 checksum)
        // ══════════════════════════════════════════════════════════════════

        // cd, cd, data58
        // "SequenceTest" -> base58Check = "BJHty2JK1hBgtcPKdzYV7h"
        {
            "cd cd data58 sequence",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 BJHty2JK1hBgtcPKdzYV7h",
            true,
            true
        },

        // data, data, xvalue sequence
        {
            "data data xvalue sequence",
            "data GNC data test xvalue",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // ISSUE 1 TEST CASE: JavaScript/C++ cross-validation
        // Uses proper base58Check encoded value with valid checksum (single SHA256)
        // "This is test data for s9-like test case with cd and regPoolEx" -> base58Check
        // ══════════════════════════════════════════════════════════════════
        {
            "Issue1 - cd data58 regPool (JavaScript compatibility)",
            "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 8T2EsZVLVHo3s91PCidKMNQAq1YLJy7apySsSVbTxoXpLJUoKttSwDe24Tw14oiTsbhF6Qd6xEDXvsLaXDXHXkWyU regPool",
            true,
            true
        },

        // ══════════════════════════════════════════════════════════════════
        // PRODUCTION TEST CASES
        // Real-world GridScript patterns used in production
        // ══════════════════════════════════════════════════════════════════

        // Token Pool Registration - real production pattern with large data58 payload
        // This tests extended length encoding (payload > 127 bytes)
        {
            "Token Pool Registration",
            "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool",
            true,
            true
        },

        // Sacrificial Transaction - real production pattern
        {
            "Sacrificial Transaction",
            "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000",
            true,
            true
        },
    };

    int passed = 0;
    int failed = 0;

    std::cout << "=== GridScript Bytecode Serialization Tests ===" << std::endl;
    std::cout << "Bytecode Version: " << compiler.mBytecodeVersion << std::endl;
    std::cout << std::endl;

    for (const auto& test : tests) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Test: " << test.name << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        std::vector<uint8_t> bytecode;
        bool compileResult = compiler.compile(test.input, bytecode);

        if (compileResult != test.shouldCompile) {
            std::cout << "FAILED: Compile " << (compileResult ? "succeeded" : "failed")
                      << " but expected " << (test.shouldCompile ? "success" : "failure") << std::endl;
            failed++;
            continue;
        }

        if (!compileResult) {
            std::cout << "PASSED: Compile correctly failed" << std::endl;
            passed++;
            continue;
        }

        std::string reconstructed;
        bool decompileResult = compiler.decompile(bytecode, reconstructed);

        if (!decompileResult) {
            std::cout << "FAILED: Decompile failed" << std::endl;
            failed++;
            continue;
        }

        if (test.shouldRoundTrip) {
            if (Tools::doStringsMatch(test.input, reconstructed)) {
                std::cout << "PASSED: Round-trip successful" << std::endl;
                passed++;
            } else {
                std::cout << "FAILED: Round-trip mismatch" << std::endl;
                std::cout << "  Expected: " << test.input.substr(0, 100) << (test.input.size() > 100 ? "..." : "") << std::endl;
                std::cout << "  Got:      " << reconstructed.substr(0, 100) << (reconstructed.size() > 100 ? "..." : "") << std::endl;

                // Find first difference
                for (size_t i = 0; i < std::min(test.input.size(), reconstructed.size()); i++) {
                    if (test.input[i] != reconstructed[i]) {
                        std::cout << "  First diff at position " << i << ": expected '" << test.input[i]
                                  << "' (0x" << std::hex << (int)(unsigned char)test.input[i] << std::dec
                                  << ") got '" << reconstructed[i]
                                  << "' (0x" << std::hex << (int)(unsigned char)reconstructed[i] << std::dec << ")" << std::endl;
                        break;
                    }
                }
                if (test.input.size() != reconstructed.size()) {
                    std::cout << "  Length diff: expected " << test.input.size() << " got " << reconstructed.size() << std::endl;
                }
                failed++;
            }
        } else {
            std::cout << "PASSED: Compile succeeded (no round-trip check)" << std::endl;
            passed++;
        }
        std::cout << std::endl;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "Summary: " << passed << " passed, " << failed << " failed" << std::endl;

    // ══════════════════════════════════════════════════════════════════════════
    // CROSS-VALIDATION: Decompile bytecode generated by JavaScript compiler
    // This tests that C++ can decompile what JavaScript produces
    // ══════════════════════════════════════════════════════════════════════════
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Cross-Validation: JavaScript -> C++ Decompilation" << std::endl;
    std::cout << "========================================" << std::endl;

    // JavaScript compiler output for "cd /test/path"
    // Base64: whAk6g6VErtzg56RW8Sswpg4Ck9KB6XbBrPgrwsDF8usgI4KL3Rlc3QvcGF0aA==
    struct CrossValidationTest {
        std::string name;
        std::string jsBase64Bytecode;
        std::string expectedDecompiled;
    };

    // UPDATED: JavaScript bytecodes with correct opcodes after +3 index offset (memory functions)
    // cd opcode: 165, data58 opcode: 145, xvalue opcode: 128, echo opcode: 160
    // setVarEx opcode: 109, getVarEx opcode: 106, regPool opcode: 301, sacrifice opcode: 206
    std::vector<CrossValidationTest> crossValidationTests = {
        // Basic cd commands (cd opcode 165 = 0xA5)
        {
            "JS: simple cd",
            "whAk6g6VErtzg56RW8Sswpg4Ck9KB6XbBrPgrwsDF8usgKUKL3Rlc3QvcGF0aA==",
            "cd /test/path"
        },
        {
            "JS: cd with address",
            "whAk6g6VErtzg56RW8Sswpg4Ck9KB6XbBrPgrwsDF8usgKUiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqbw==",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo"
        },
        {
            "JS: cd with / path",
            "whAk6g6VErtzg56RW8Sswpg4Ck9KB6XbBrPgrwsDF8usgKUBLw==",
            "cd /"
        },
        {
            "JS: multiple cd commands",
            "wm2RjFrN0W0lQoIGIoBqIhcP2BYbauaqmI2EY1U+JabQgKUiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqb4ClAS8=",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd /"
        },
        // Basic data commands (data opcode unchanged at 11)
        {
            "JS: simple data",
            "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YToCwNHTkM=",
            "data GNC"
        },
        {
            "JS: multiple data",
            "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YToCwNHTkMLIjE1WnRxZlVXNXdqdjIzU0s3SjlGM2tiaEMybnhMZHNuS1YLATA=",
            "data GNC data 15ZtqfUW5wjv23SK7J9F3kbhC2nxLdsnKV data 0"
        },
        {
            "JS: xvalue command",
            "wgmSrptXyv0V4EHNiZABL/QPKcIRudpJqmtjajE4uXMdCwNHTkOAgA==",
            "data GNC xvalue"
        },
        // data58 tests (data58 opcode 145 = 0x91)
        {
            "JS: simple data58",
            "wiog+86lvQXJR+wVKllsyDlF/kWoZZTLReDRne1pWn8YgJEIVGVzdERhdGE=",
            "data58 2bNcNLCKuxzGWJ5VP"
        },
        {
            "JS: data58 larger payload",
            "wiog+86lvQXJR+wVKllsyDlF/kWoZZTLReDRne1pWn8YgJEwVGhpcyBpcyBhIGxhcmdlciB0ZXN0IHBheWxvYWQgZm9yIGRhdGE1OCBjb21tYW5k",
            "data58 MG97WaBEMjd3YwtxH7SMKZrLYewZzN3t9FfCvnZxEc3UBdvQWhgBUSCKpLton3QD6sHbJ2A"
        },
        // data64 tests (data64 opcode unchanged at 9)
        {
            "JS: data64 simple",
            "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YToCQtIZWxsbyBXb3JsZA==",
            "data64 SGVsbG8gV29ybGRCqHOs"
        },
        {
            "JS: data64 longer content",
            "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YToCTBUaGlzIGlzIGEgbG9uZ2VyIHRlc3Qgc3RyaW5nIGZvciBiYXNlNjQgZW5jb2Rpbmc=",
            "data64 VGhpcyBpcyBhIGxvbmdlciB0ZXN0IHN0cmluZyBmb3IgYmFzZTY0IGVuY29kaW5nhCrGaw=="
        },
        // regPool tests (regPool opcode 301 = 0x812D)
        {
            "JS: regPool without inline param",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0A",
            "regPool"
        },
        {
            "JS: regPool with inline param",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0RVGVzdCByZWdQb29sIGRhdGE=",
            "regPool VGVzdCByZWdQb29sIGRhdGFRFbrn"
        },
        // Flag tests (-flag)
        {
            "JS: -t flag (single char)",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0OLXQgSGVsbG8gV29ybGQ=",
            "regPool -t SGVsbG8gV29ybGRCqHOs"
        },
        {
            "JS: -help flag (multi char)",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0KLWhlbHAgSGVscA==",
            "regPool -help SGVscHND5n4="
        },
        {
            "JS: -page123 flag (with numbers)",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0NLXBhZ2UxMjMgUGFnZQ==",
            "regPool -page123 UGFnZQ4g+mo="
        },
        // Flag tests (+flag)
        {
            "JS: +t flag (single char)",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0OK3QgSGVsbG8gV29ybGQ=",
            "regPool +t SGVsbG8gV29ybGRCqHOs"
        },
        {
            "JS: +verbose flag (multi char)",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0QK3ZlcmJvc2UgVmVyYm9zZQ==",
            "regPool +verbose VmVyYm9zZU1032E="
        },
        {
            "JS: +opt42 flag (with numbers)",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0NK29wdDQyIE9wdGlvbg==",
            "regPool +opt42 T3B0aW9u3OJhLg=="
        },
        // Long flag tests
        {
            "JS: long minus flag -verylongflagname",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0WLXZlcnlsb25nZmxhZ25hbWUgTG9uZw==",
            "regPool -verylongflagname TG9uZ6fWI/0="
        },
        {
            "JS: long plus flag +verylongflagname",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0WK3Zlcnlsb25nZmxhZ25hbWUgTG9uZw==",
            "regPool +verylongflagname TG9uZ6fWI/0="
        },
        {
            "JS: flag with numbers -opt1234567890",
            "wlu5tWOH8M14Tw07mCyRXc01Km08NqPcx7yA+aK33b/cgS0WLW9wdDEyMzQ1Njc4OTAgTnVtYmVycw==",
            "regPool -opt1234567890 TnVtYmVyczGtoUw="
        },
        // Sequence tests
        {
            "JS: data data sequence",
            "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YToCwNHTkMLBHRlc3Q=",
            "data GNC data test"
        },
        {
            "JS: cd cd data58 sequence",
            "wufQTTr/VnaH0CdS0a5TuaqjGdmb59mV8rtRlgWH5sBZgKUiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqb4ClAS+AkQxTZXF1ZW5jZVRlc3Q=",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 BJHty2JK1hBgtcPKdzYV7h"
        },
        // Complex sequences (s9-like, s15-like, s16-like tests)
        {
            "JS: s9-like - cd data58 regPoolEx",
            "wtxQqpO7nNntV4lFtx9orsZyw7NtxQiJoTM+Bi3LdNYwgKUiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqb4ClAS+AkT1UaGlzIGlzIHRlc3QgZGF0YSBmb3IgczktbGlrZSB0ZXN0IGNhc2Ugd2l0aCBjZCBhbmQgcmVnUG9vbEV4gOA=",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 8T2EsZVLVHo3s91PCidKMNQAq1YLJy7apySsSVbTxoXpLJUoKttSwDe24Tw14oiTsbhF6Qd6xEDXvsLaXDXHXkWyU regPoolEx"
        },
        {
            "JS: s15-like - regPool without inline param",
            "wjx5X+C/OjWndhZxUxzlSCQay0fvKHMOchBu7TYVtjKegKUiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqb4ClAS+AkT1UaGlzIGlzIHRlc3QgZGF0YSBmb3IgczktbGlrZSB0ZXN0IGNhc2Ugd2l0aCBjZCBhbmQgcmVnUG9vbEV4gS0A",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 8T2EsZVLVHo3s91PCidKMNQAq1YLJy7apySsSVbTxoXpLJUoKttSwDe24Tw14oiTsbhF6Qd6xEDXvsLaXDXHXkWyU regPool"
        },
        {
            "JS: s16-like - regPool with -t flag",
            "wqhbOnDf5A6zSSuVLrXM+QYpqknFYRpk92QoVAbh8eMbgKUiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqb4ClAS+BLRktdCBUZXN0IHBheWxvYWQgd2l0aCBmbGFn",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / regPool -t VGVzdCBwYXlsb2FkIHdpdGggZmxhZ5qOpkk="
        },
        {
            "JS: s16 variant - regPool with +t flag",
            "wqhbOnDf5A6zSSuVLrXM+QYpqknFYRpk92QoVAbh8eMbgKUiMUFCNldGM3kzZTV5OWhtWUFZdlBlSHFYbWNib1lrY3Vqb4ClAS+BLRkrdCBUZXN0IHBheWxvYWQgd2l0aCBmbGFn",
            "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / regPool +t VGVzdCBwYXlsb2FkIHdpdGggZmxhZ5qOpkk="
        },
        // Canonical case tests
        {
            "JS: setVarEx canonical case",
            "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YTobQ==",
            "setVarEx"
        },
        {
            "JS: getVarEx canonical case",
            "wpb96UZXw8rgHVvr3urRixC7UT7JrJxFk7HMWpDM1YToag==",
            "getVarEx"
        },
        {
            "JS: echo canonical case",
            "whee2iDHC+QzBbWaAC5+Zs6co+nlDrhtCEL+emsA/KHFCwNHTkOAoAA=",
            "data GNC echo"
        },
        // Production test cases - Token Pool Registration and Sacrificial Transaction
        {
            "JS: Token Pool Registration",
            "wtzjzPexEv4/8yH8S+oTujd01nGO8kxZDDx7twCCKzOCgKUjLzE0NGMyQW9mNXQzR0RVVHY4eDltUG9ycHpNTGhrRlFMV3CAkYILATCCAQcCAQIwggEABCIxNDRjMkFvZjV0M0dEVVR2OHg5bVBvcnB6TUxoa0ZRTFdwBAAEASICAQMEA8CjmwQhAXpHO7GBQNZaQ8hKoM4y2rPMzzRUO2aY3SJgRPzxJJ9EBCCBgKTdmaGlc9X0nTF3F7UVYmdDoi2yzYu9NfbOx5n8KgQIcGF5bWVudHMCAQAwezAnBCAJfthldb8KyiENmlK/k/jSv/c7u/Q7yZfPjoAEHHbmhAQABAEAMCcEILJzy6vRPvyf0vHh6UaAi5EEXhhGuXJmVLOFSrSYsIu6BAAEAQAwJwQgtgI1MxoQOwoAz3R8uLfapYcZZdCcsZBxSSBW8EwH1joEAAQBAIEtAA==",
            "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool"
        },
        {
            "JS: Sacrificial Transaction",
            "wnSx3I5FOWDFLG7pdMYdqI9f6oPQKDvH8t9pOKZDXPeEgKUjLzE0NGMyQW9mNXQzR0RVVHY4eDltUG9ycHpNTGhrRlFMV3CAzggxNDQwMDAwMA==",
            "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000"
        },
    };

    int crossPassed = 0;
    int crossFailed = 0;

    for (const auto& cvTest : crossValidationTests) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Cross-validation: " << cvTest.name << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        // Decode base64 bytecode from JavaScript
        std::vector<uint8_t> jsBytecode;
        if (!Tools::base64Decode(cvTest.jsBase64Bytecode, jsBytecode)) {
            std::cout << "FAILED: Could not decode JavaScript bytecode" << std::endl;
            crossFailed++;
            continue;
        }

        std::cout << "  JS bytecode size: " << jsBytecode.size() << " bytes" << std::endl;

        // Decompile using C++ decompiler
        std::string decompiled;
        bool decompileResult = compiler.decompile(jsBytecode, decompiled);

        if (!decompileResult) {
            std::cout << "FAILED: C++ decompilation failed" << std::endl;
            crossFailed++;
            continue;
        }

        std::cout << "  Decompiled: \"" << decompiled << "\"" << std::endl;

        if (decompiled == cvTest.expectedDecompiled) {
            std::cout << "PASSED: Cross-validation successful" << std::endl;
            crossPassed++;
        } else {
            std::cout << "FAILED: Decompilation mismatch" << std::endl;
            std::cout << "  Expected: \"" << cvTest.expectedDecompiled << "\"" << std::endl;
            std::cout << "  Got:      \"" << decompiled << "\"" << std::endl;
            crossFailed++;
        }
        std::cout << std::endl;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "Cross-validation Summary: " << crossPassed << " passed, " << crossFailed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    // ══════════════════════════════════════════════════════════════════════════
    // VERIFY SHA256 IMPLEMENTATION
    // ══════════════════════════════════════════════════════════════════════════
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "SHA256 Implementation Verification" << std::endl;
    std::cout << "========================================" << std::endl;

    // Test vector: SHA256("test") should equal 9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08
    std::string testStr = "test";
    std::vector<uint8_t> testBytes(testStr.begin(), testStr.end());
    std::vector<uint8_t> testHash = SHA256::hash(testBytes);
    std::cout << "SHA256(\"test\") = ";
    for (auto b : testHash) std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)b;
    std::cout << std::dec << std::endl;
    std::cout << "Expected:        9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08" << std::endl;

    // Test seed hash
    std::string seedStr = "GRIDSCRIPT_V2_KEYWORD_IMAGE";
    std::vector<uint8_t> seedBytes(seedStr.begin(), seedStr.end());
    std::vector<uint8_t> seedHash = SHA256::hash(seedBytes);
    std::cout << "SHA256(\"GRIDSCRIPT_V2_KEYWORD_IMAGE\") = ";
    for (auto b : seedHash) std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)b;
    std::cout << std::dec << std::endl;

    // ══════════════════════════════════════════════════════════════════════════
    // REVERSE CROSS-VALIDATION OUTPUT: C++ -> JavaScript
    // Generate V2 bytecode that JavaScript can use for reverse testing
    // ══════════════════════════════════════════════════════════════════════════
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "C++ Bytecode for JavaScript Decompilation Testing" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Copy these test vectors to JavaScript test file:" << std::endl;
    std::cout << std::endl;

    // Test cases for reverse validation - compile with V2 header
    std::vector<std::pair<std::string, std::string>> reverseTestCases = {
        {"CPP: simple cd", "cd /test/path"},
        {"CPP: cd with address", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo"},
        {"CPP: simple data", "data GNC"},
        {"CPP: multiple data", "data GNC data test"},
        {"CPP: xvalue command", "data GNC xvalue"},
        {"CPP: simple data58", "data58 2bNcNLCKuxzGWJ5VP"},
        {"CPP: data64 simple", "data64 SGVsbG8gV29ybGRCqHOs"},
        {"CPP: regPool with flag", "regPool -t SGVsbG8gV29ybGRCqHOs"},
        {"CPP: cd cd data58 sequence", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 BJHty2JK1hBgtcPKdzYV7h"},
        // Production test cases
        {"CPP: Token Pool Registration", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool"},
        {"CPP: Sacrificial Transaction", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000"},
    };

    std::cout << "const cppBytecodeTests = [" << std::endl;
    for (const auto& tc : reverseTestCases) {
        std::vector<uint8_t> bytecodeWithHeader;
        if (compiler.compileWithHeader(tc.second, bytecodeWithHeader)) {
            std::string base64Bytecode = Tools::base64Encode(bytecodeWithHeader);
            std::cout << "    { name: \"" << tc.first << "\", bytecode: \"" << base64Bytecode << "\", expected: \"" << tc.second << "\" }," << std::endl;
        } else {
            std::cout << "    // FAILED to compile: " << tc.first << std::endl;
        }
    }
    std::cout << "];" << std::endl;

    return (failed > 0 || crossFailed > 0) ? 1 : 0;
}
