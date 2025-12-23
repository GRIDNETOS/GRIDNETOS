/**
 * @file GridScriptCompiler_Standalone.cpp
 * @brief Standalone implementation of CGridScriptCompiler for unit testing
 *
 * This file provides a self-contained implementation of the GridScript compiler
 * that doesn't require the full GRIDNET dependencies. It implements the same
 * compile/decompile algorithms as the production GridScriptCompiler.cpp.
 *
 * Usage: Compile with -DGRIDSCRIPT_STANDALONE_TEST
 *
 * IMPORTANT: This implementation MUST produce identical bytecode to the
 * production compiler. Any changes to bytecode format should be reflected here.
 */

#ifdef GRIDSCRIPT_STANDALONE_TEST

#include "GridScriptCompiler.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

// ═══════════════════════════════════════════════════════════════════════════════
// SHA256 Implementation (matching production CryptoFactory)
// ═══════════════════════════════════════════════════════════════════════════════
namespace {
    // SHA256 constants
    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    uint32_t ep0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    uint32_t ep1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
        uint32_t h[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        // Pre-processing: adding padding bits
        std::vector<uint8_t> msg = data;
        uint64_t bitLen = msg.size() * 8;
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) msg.push_back(0x00);

        // Append length in big-endian
        for (int i = 7; i >= 0; i--) msg.push_back((bitLen >> (i * 8)) & 0xff);

        // Process each 64-byte chunk
        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; i++) {
                w[i] = (msg[chunk + i * 4] << 24) | (msg[chunk + i * 4 + 1] << 16) |
                       (msg[chunk + i * 4 + 2] << 8) | msg[chunk + i * 4 + 3];
            }
            for (int i = 16; i < 64; i++) {
                w[i] = ep1(w[i - 2]) + w[i - 7] + ep0(w[i - 15]) + w[i - 16];
            }

            uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
            uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

            for (int i = 0; i < 64; i++) {
                uint32_t t1 = hh + sig1(e) + ch(e, f, g) + K[i] + w[i];
                uint32_t t2 = sig0(a) + maj(a, b, c);
                hh = g; g = f; f = e; e = d + t1;
                d = c; c = b; b = a; a = t1 + t2;
            }

            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        std::vector<uint8_t> hash(32);
        for (int i = 0; i < 8; i++) {
            hash[i * 4] = (h[i] >> 24) & 0xff;
            hash[i * 4 + 1] = (h[i] >> 16) & 0xff;
            hash[i * 4 + 2] = (h[i] >> 8) & 0xff;
            hash[i * 4 + 3] = h[i] & 0xff;
        }
        return hash;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Base58 Implementation (matching production CTools)
// ═══════════════════════════════════════════════════════════════════════════════
namespace {
    const char* BASE58_CHARS = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    std::string base58Encode(const std::vector<uint8_t>& data) {
        if (data.empty()) return "";

        // Count leading zeros
        size_t zeroes = 0;
        for (size_t i = 0; i < data.size() && data[i] == 0; i++) zeroes++;

        // Allocate enough space
        std::vector<uint8_t> b58(data.size() * 138 / 100 + 1, 0);
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

        // Skip leading zeroes in b58
        auto it = b58.begin() + (b58.size() - length);
        std::string result(zeroes, '1');
        while (it != b58.end()) {
            result += BASE58_CHARS[*it++];
        }
        return result;
    }

    bool base58Decode(const std::string& encoded, std::vector<uint8_t>& output) {
        if (encoded.empty()) return false;

        // Count leading '1's
        size_t zeroes = 0;
        for (size_t i = 0; i < encoded.size() && encoded[i] == '1'; i++) zeroes++;

        // Allocate enough space
        std::vector<uint8_t> b256(encoded.size() * 733 / 1000 + 1, 0);
        size_t length = 0;

        for (size_t i = zeroes; i < encoded.size(); i++) {
            const char* p = strchr(BASE58_CHARS, encoded[i]);
            if (p == nullptr) return false;
            int carry = static_cast<int>(p - BASE58_CHARS);
            size_t j = 0;
            for (auto it = b256.rbegin(); (carry != 0 || j < length) && it != b256.rend(); ++it, ++j) {
                carry += 58 * (*it);
                *it = carry % 256;
                carry /= 256;
            }
            length = j;
        }

        // Skip leading zeroes in b256
        auto it = b256.begin() + (b256.size() - length);
        output.assign(zeroes, 0);
        while (it != b256.end()) {
            output.push_back(*it++);
        }
        return true;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Base64 Implementation (matching production CTools)
// ═══════════════════════════════════════════════════════════════════════════════
namespace {
    const char* BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string base64Encode(const std::vector<uint8_t>& data) {
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

    bool base64Decode(const std::string& encoded, std::vector<uint8_t>& output) {
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[static_cast<unsigned char>(BASE64_CHARS[i])] = i;

        output.clear();
        int val = 0, valb = -8;
        for (char c : encoded) {
            if (c == '=') break;
            if (T[static_cast<unsigned char>(c)] == -1) continue;
            val = (val << 6) + T[static_cast<unsigned char>(c)];
            valb += 6;
            if (valb >= 0) {
                output.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return true;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// CGridScriptCompiler Implementation (Standalone Mode)
// ═══════════════════════════════════════════════════════════════════════════════

CGridScriptCompiler::CGridScriptCompiler(const std::list<SE::Definition>& definitions, uint64_t bytecodeVersion)
    : mStandaloneDefinitions(definitions), mBytecodeVersion(bytecodeVersion), mCurrentOffset(0), mCompilingExtendedID(false)
{
}

std::vector<uint8_t> CGridScriptCompiler::computeSHA256(const std::vector<uint8_t>& data) {
    return sha256(data);
}

bool CGridScriptCompiler::initializeKeywordHash() {
    std::string initStr = GRIDSCRIPT_IMAGE_INIT_STRING;
    std::vector<uint8_t> initData(initStr.begin(), initStr.end());
    mCurrentImageHash = computeSHA256(initData);
    return true;
}

bool CGridScriptCompiler::updateKeywordHash(const std::string& keywordName) {
    std::vector<uint8_t> combined = mCurrentImageHash;
    combined.insert(combined.end(), keywordName.begin(), keywordName.end());
    mCurrentImageHash = computeSHA256(combined);
    return true;
}

std::vector<uint8_t> CGridScriptCompiler::getFinalKeywordHash() const {
    return mCurrentImageHash;
}

// Base58Check: Single SHA256 checksum (matching reference CTools in Tools.cpp)
std::string CGridScriptCompiler::base58CheckEncode(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> checksum = sha256(data);
    std::vector<uint8_t> combined = data;
    combined.insert(combined.end(), checksum.begin(), checksum.begin() + 4);
    return base58Encode(combined);
}

bool CGridScriptCompiler::base58CheckDecode(const std::string& encoded, std::vector<uint8_t>& output) {
    std::vector<uint8_t> decoded;
    if (!base58Decode(encoded, decoded)) return false;
    if (decoded.size() < 5) return false;

    output.assign(decoded.begin(), decoded.end() - 4);
    std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());
    std::vector<uint8_t> expectedChecksum = sha256(output);

    for (int i = 0; i < 4; i++) {
        if (checksum[i] != expectedChecksum[i]) return false;
    }
    return true;
}

// Base64Check: Double SHA256 checksum
std::string CGridScriptCompiler::base64CheckEncode(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash1 = sha256(data);
    std::vector<uint8_t> checksum = sha256(hash1);
    std::vector<uint8_t> combined = data;
    combined.insert(combined.end(), checksum.begin(), checksum.begin() + 4);
    return base64Encode(combined);
}

bool CGridScriptCompiler::base64CheckDecode(const std::string& encoded, std::vector<uint8_t>& output) {
    std::vector<uint8_t> decoded;
    if (!base64Decode(encoded, decoded)) return false;
    if (decoded.size() < 5) return false;

    output.assign(decoded.begin(), decoded.end() - 4);
    std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());
    std::vector<uint8_t> hash1 = sha256(output);
    std::vector<uint8_t> expectedChecksum = sha256(hash1);

    for (int i = 0; i < 4; i++) {
        if (checksum[i] != expectedChecksum[i]) return false;
    }
    return true;
}

bool CGridScriptCompiler::setIDBits(uint64_t id) {
    if (id > 0x7FFF) return false;  // Max 15 bits (2 bytes with high bit marker)

    if (id > 127) {
        mCompilingExtendedID = true;
        mCurrentOpCode.resize(2);
        mCurrentOpCode[0] = static_cast<uint8_t>((id >> 8) | 0x80);
        mCurrentOpCode[1] = static_cast<uint8_t>(id & 0xFF);
    } else {
        mCompilingExtendedID = false;
        mCurrentOpCode.resize(1);
        mCurrentOpCode[0] = static_cast<uint8_t>(id);
    }
    return true;
}

bool CGridScriptCompiler::getIDBits(uint64_t& ID, bool movePointer) {
    if (mCurrentOffset >= mCompiledByteCode.size()) return false;

    uint8_t firstByte = mCompiledByteCode[mCurrentOffset];

    if (firstByte & 0x80) {
        // Extended encoding (2 bytes)
        if (mCurrentOffset + 1 >= mCompiledByteCode.size()) return false;
        ID = ((firstByte & 0x7F) << 8) | mCompiledByteCode[mCurrentOffset + 1];
        if (movePointer) mCurrentOffset += 2;
    } else {
        // Simple encoding (1 byte)
        ID = firstByte;
        if (movePointer) mCurrentOffset += 1;
    }
    return true;
}

bool CGridScriptCompiler::setLengthBits(size_t size, bool& wasExtLengthEncodingUsed) {
    if (size <= 127) {
        wasExtLengthEncodingUsed = false;
        mCurrentOpCode.push_back(static_cast<uint8_t>(size));
    } else {
        wasExtLengthEncodingUsed = true;
        // Extended length: high bit set, followed by 2-byte length
        size_t remaining = size;
        std::vector<uint8_t> lengthBytes;
        while (remaining > 0) {
            lengthBytes.push_back(static_cast<uint8_t>(remaining & 0xFF));
            remaining >>= 8;
        }
        mCurrentOpCode.push_back(static_cast<uint8_t>(0x80 | lengthBytes.size()));
        for (auto it = lengthBytes.rbegin(); it != lengthBytes.rend(); ++it) {
            mCurrentOpCode.push_back(*it);
        }
    }
    return true;
}

bool CGridScriptCompiler::getContentLength(uint64_t& contentLength, bool movePointer) {
    if (mCurrentOffset >= mCompiledByteCode.size()) return false;

    uint8_t firstByte = mCompiledByteCode[mCurrentOffset];
    size_t bytesRead = 1;

    if (firstByte & 0x80) {
        // Extended length encoding
        size_t numBytes = firstByte & 0x7F;
        if (mCurrentOffset + 1 + numBytes > mCompiledByteCode.size()) return false;

        contentLength = 0;
        for (size_t i = 0; i < numBytes; i++) {
            contentLength = (contentLength << 8) | mCompiledByteCode[mCurrentOffset + 1 + i];
        }
        bytesRead = 1 + numBytes;
    } else {
        contentLength = firstByte;
    }

    if (movePointer) mCurrentOffset += bytesRead;
    return true;
}

bool CGridScriptCompiler::setContentBits(std::vector<uint8_t> content) {
    mCurrentOpCode.insert(mCurrentOpCode.end(), content.begin(), content.end());
    return true;
}

bool CGridScriptCompiler::setContentBits(uint64_t numerical) {
    // Convert number to minimal byte representation
    std::vector<uint8_t> bytes;
    uint64_t val = numerical;
    do {
        bytes.push_back(static_cast<uint8_t>(val & 0xFF));
        val >>= 8;
    } while (val > 0);
    std::reverse(bytes.begin(), bytes.end());
    return setContentBits(bytes);
}

bool CGridScriptCompiler::getContentBits(std::vector<uint8_t>& content, uint64_t length, bool movePointer) {
    if (mCurrentOffset + length > mCompiledByteCode.size()) return false;
    content.assign(mCompiledByteCode.begin() + mCurrentOffset,
                   mCompiledByteCode.begin() + mCurrentOffset + length);
    if (movePointer) mCurrentOffset += length;
    return true;
}

bool CGridScriptCompiler::storeInlineLiteral(std::string& data) {
    std::vector<uint8_t> bytes(data.begin(), data.end());
    bool extUsed;
    setLengthBits(bytes.size(), extUsed);
    setContentBits(bytes);
    return true;
}

bool CGridScriptCompiler::storeUserOpCode(uint64_t lowerLevelOpCodeIDToUse) {
    setIDBits(BYTECODE_ID_USER_OPCODE);
    std::vector<uint8_t> idBytes;
    uint64_t val = lowerLevelOpCodeIDToUse;
    do {
        idBytes.push_back(static_cast<uint8_t>(val & 0xFF));
        val >>= 8;
    } while (val > 0);
    bool extUsed;
    setLengthBits(idBytes.size(), extUsed);
    setContentBits(idBytes);
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main compile() implementation
// ═══════════════════════════════════════════════════════════════════════════════
bool CGridScriptCompiler::compile(std::string code, std::vector<uint8_t>& result) {
    std::lock_guard<std::mutex> lock(mGuardian);

    // Initialize hash chain for V2+
    if (mBytecodeVersion >= 2) {
        if (!initializeKeywordHash()) return false;
    }

    std::map<std::string, uint8_t> userDefinedCMDs;
    uint64_t lastGeneratedUserOpCodeID = 0;
    std::list<SE::Definition>& definitions = getDefinitions();

    std::regex wordRegex("\\S+");
    std::regex unsignedNrR("^\\d+$");
    std::regex signedNrR("^\\-\\d+$");
    std::regex doubleR("^\\d+\\.\\d+$");
    std::regex flagR("^[\\-\\+][a-zA-Z0-9]+$");

    mCurrentOffset = 0;
    mCompiledByteCode.clear();
    mCurrentOpCode.clear();

    std::sregex_iterator it(code.begin(), code.end(), wordRegex);
    std::sregex_iterator end;

    SE::Definition* activeDefinition = nullptr;
    int waitingForParams = 0;
    std::string flagPrefix;

    while (it != end) {
        std::string token = it->str();
        ++it;

        // Case-insensitive lookup
        std::string tokenLower = token;
        std::transform(tokenLower.begin(), tokenLower.end(), tokenLower.begin(), ::tolower);

        // Check if waiting for inline parameters
        if (waitingForParams > 0 && activeDefinition != nullptr) {
            // Check for flag prefix
            if (std::regex_match(token, flagR) && (activeDefinition->hasBase58BinaryInLineParams || activeDefinition->hasBase64BinaryInLineParams)) {
                flagPrefix = token + " ";
                // Next token will be the actual parameter
                continue;
            }

            std::vector<uint8_t> paramBytes;

            if (activeDefinition->hasBase58BinaryInLineParams) {
                if (!base58CheckDecode(token, paramBytes)) return false;
            } else if (activeDefinition->hasBase64BinaryInLineParams) {
                if (!base64CheckDecode(token, paramBytes)) return false;
            } else {
                paramBytes.assign(token.begin(), token.end());
            }

            // Prepend flag if present
            if (!flagPrefix.empty()) {
                std::vector<uint8_t> flagBytes(flagPrefix.begin(), flagPrefix.end());
                flagBytes.insert(flagBytes.end(), paramBytes.begin(), paramBytes.end());
                paramBytes = flagBytes;
                flagPrefix.clear();
            }

            bool extUsed;
            setLengthBits(paramBytes.size(), extUsed);
            setContentBits(paramBytes);

            // Commit opcode
            mCompiledByteCode.insert(mCompiledByteCode.end(), mCurrentOpCode.begin(), mCurrentOpCode.end());
            mCurrentOpCode.clear();

            waitingForParams--;
            if (waitingForParams == 0) activeDefinition = nullptr;
            continue;
        }

        // Find definition (case-insensitive)
        activeDefinition = nullptr;
        for (auto& def : definitions) {
            std::string defNameLower = def.name;
            std::transform(defNameLower.begin(), defNameLower.end(), defNameLower.begin(), ::tolower);
            if (defNameLower == tokenLower) {
                activeDefinition = &def;
                break;
            }
        }

        if (activeDefinition != nullptr) {
            // Known command
            setIDBits(activeDefinition->id);

            // Update hash chain for V2+
            if (mBytecodeVersion >= 2 && activeDefinition->id > BYTECODE_ID_STRING_LITERAL) {
                if (!updateKeywordHash(activeDefinition->name)) return false;
            }

            if (activeDefinition->extType && activeDefinition->inlineParamCount > 0) {
                waitingForParams = activeDefinition->inlineParamCount;
                // Don't commit yet - wait for params
            } else {
                // Primitive opcode - commit immediately
                mCompiledByteCode.insert(mCompiledByteCode.end(), mCurrentOpCode.begin(), mCurrentOpCode.end());
                mCurrentOpCode.clear();
                activeDefinition = nullptr;
            }
        } else if (std::regex_match(token, unsignedNrR)) {
            // Unsigned number
            setIDBits(BYTECODE_ID_UNSIGNED);
            uint64_t num = std::stoull(token);
            std::vector<uint8_t> numBytes;
            do {
                numBytes.insert(numBytes.begin(), static_cast<uint8_t>(num & 0xFF));
                num >>= 8;
            } while (num > 0);
            bool extUsed;
            setLengthBits(numBytes.size(), extUsed);
            setContentBits(numBytes);
            mCompiledByteCode.insert(mCompiledByteCode.end(), mCurrentOpCode.begin(), mCurrentOpCode.end());
            mCurrentOpCode.clear();
        } else if (std::regex_match(token, signedNrR)) {
            // Signed number
            setIDBits(BYTECODE_ID_SIGNED);
            int64_t num = std::stoll(token);
            std::vector<uint8_t> numBytes;
            bool negative = num < 0;
            uint64_t absVal = negative ? static_cast<uint64_t>(-num) : static_cast<uint64_t>(num);
            do {
                numBytes.insert(numBytes.begin(), static_cast<uint8_t>(absVal & 0xFF));
                absVal >>= 8;
            } while (absVal > 0);
            if (negative) {
                // Two's complement for negative
                for (auto& b : numBytes) b = ~b;
                for (int i = numBytes.size() - 1; i >= 0; i--) {
                    if (++numBytes[i] != 0) break;
                }
            }
            bool extUsed;
            setLengthBits(numBytes.size(), extUsed);
            setContentBits(numBytes);
            mCompiledByteCode.insert(mCompiledByteCode.end(), mCurrentOpCode.begin(), mCurrentOpCode.end());
            mCurrentOpCode.clear();
        } else {
            // Unknown token - treat as string literal
            setIDBits(BYTECODE_ID_STRING_LITERAL);
            std::vector<uint8_t> strBytes(token.begin(), token.end());
            bool extUsed;
            setLengthBits(strBytes.size(), extUsed);
            setContentBits(strBytes);
            mCompiledByteCode.insert(mCompiledByteCode.end(), mCurrentOpCode.begin(), mCurrentOpCode.end());
            mCurrentOpCode.clear();
        }
    }

    result = mCompiledByteCode;
    return !result.empty();
}

// ═══════════════════════════════════════════════════════════════════════════════
// compileWithHeader() - adds V2 header
// ═══════════════════════════════════════════════════════════════════════════════
bool CGridScriptCompiler::compileWithHeader(const std::string& sourceCode, std::vector<uint8_t>& result) {
    if (!compile(sourceCode, result)) return false;

    if (mBytecodeVersion >= 2) {
        // Prepend V2 header: version byte + 32-byte hash
        std::vector<uint8_t> header;
        header.push_back(static_cast<uint8_t>(192 + mBytecodeVersion));  // Version marker
        std::vector<uint8_t> hash = getFinalKeywordHash();
        header.insert(header.end(), hash.begin(), hash.end());
        header.insert(header.end(), result.begin(), result.end());
        result = header;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main decompile() implementation
// ═══════════════════════════════════════════════════════════════════════════════
bool CGridScriptCompiler::decompile(std::vector<uint8_t> bytecode, std::string& sourceCode, const std::vector<std::string>& codeWords) {
    std::lock_guard<std::mutex> lock(mGuardian);

    if (bytecode.empty()) return false;

    // Detect bytecode version
    mBytecodeVersion = 1;
    std::vector<uint8_t> extractedHash;
    mCurrentOffset = 0;

    if (bytecode[0] >= 192) {
        mBytecodeVersion = 2;
        if (bytecode.size() < 33) return false;
        extractedHash.assign(bytecode.begin() + 1, bytecode.begin() + 33);
        if (!initializeKeywordHash()) return false;
        mCurrentOffset = 33;
    }

    mCompiledByteCode = bytecode;
    std::list<SE::Definition>& definitions = getDefinitions();
    std::vector<std::string> resultWords;
    std::map<uint64_t, std::string> userDefinedCMDs;

    while (mCurrentOffset < mCompiledByteCode.size()) {
        uint64_t byteCodeID = 0;
        if (!getIDBits(byteCodeID, true)) return false;

        SE::Definition* activeDefinition = nullptr;
        bool isPushNumber = false;
        bool isUserDefinedOpcode = false;
        bool isInlineLiteral = false;

        // Check special opcodes
        if (byteCodeID == BYTECODE_ID_UNSIGNED || byteCodeID == BYTECODE_ID_SIGNED || byteCodeID == BYTECODE_ID_DOUBLE) {
            isPushNumber = true;
        } else if (byteCodeID == BYTECODE_ID_USER_OPCODE) {
            isUserDefinedOpcode = true;
        } else if (byteCodeID == BYTECODE_ID_STRING_LITERAL) {
            isInlineLiteral = true;
        } else {
            // Look up in definitions
            for (auto& def : definitions) {
                if (def.id == byteCodeID) {
                    activeDefinition = &def;
                    break;
                }
            }
        }

        if (isPushNumber || isUserDefinedOpcode || isInlineLiteral ||
            (activeDefinition != nullptr && activeDefinition->extType)) {
            // Read content
            uint64_t contentLength = 0;
            if (!getContentLength(contentLength, true)) return false;

            std::vector<uint8_t> content;
            if (!getContentBits(content, contentLength, true)) return false;

            if (isPushNumber) {
                // Convert bytes to number string
                uint64_t num = 0;
                for (auto b : content) num = (num << 8) | b;
                resultWords.push_back(std::to_string(num));
            } else if (isUserDefinedOpcode) {
                uint64_t opcodeId = 0;
                for (auto b : content) opcodeId = (opcodeId << 8) | b;
                if (userDefinedCMDs.find(opcodeId) != userDefinedCMDs.end()) {
                    resultWords.push_back(userDefinedCMDs[opcodeId]);
                } else {
                    std::string name = "v" + std::to_string(opcodeId);
                    userDefinedCMDs[opcodeId] = name;
                    resultWords.push_back(name);
                }
            } else if (isInlineLiteral || (activeDefinition != nullptr && activeDefinition->extType)) {
                if (content.size() > 0) {
                    if (activeDefinition != nullptr) {
                        resultWords.push_back(activeDefinition->name);

                        // Update hash chain for V2+
                        if (mBytecodeVersion >= 2 && activeDefinition->id > BYTECODE_ID_STRING_LITERAL) {
                            if (!updateKeywordHash(activeDefinition->name)) return false;
                        }
                    }

                    // Handle binary parameters with flag support
                    if (!isInlineLiteral && activeDefinition != nullptr &&
                        (activeDefinition->hasBase58BinaryInLineParams || activeDefinition->hasBase64BinaryInLineParams)) {

                        std::string flagPrefix;
                        std::vector<uint8_t> binaryContent = content;

                        // V2+ flag detection
                        if (mBytecodeVersion >= 2 && content.size() > 3 &&
                            (content[0] == '-' || content[0] == '+')) {
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
                                    if (validFlag) spacePos = i;
                                    break;
                                }
                            }
                            if (spacePos > 0) {
                                flagPrefix = std::string(content.begin(), content.begin() + spacePos + 1);
                                binaryContent = std::vector<uint8_t>(content.begin() + spacePos + 1, content.end());
                            }
                        }

                        std::string encoded = activeDefinition->hasBase58BinaryInLineParams ?
                            base58CheckEncode(binaryContent) : base64CheckEncode(binaryContent);
                        resultWords.push_back(flagPrefix + encoded);
                    } else {
                        resultWords.push_back(std::string(content.begin(), content.end()));
                    }
                } else if (mBytecodeVersion >= 2 && activeDefinition != nullptr) {
                    resultWords.push_back(activeDefinition->name);
                    if (activeDefinition->id > BYTECODE_ID_STRING_LITERAL) {
                        if (!updateKeywordHash(activeDefinition->name)) return false;
                    }
                } else {
                    return false;
                }
            }
        } else if (activeDefinition != nullptr) {
            // Primitive opcode
            resultWords.push_back(activeDefinition->name);

            // Update hash chain for V2+
            if (mBytecodeVersion >= 2 && activeDefinition->id > BYTECODE_ID_STRING_LITERAL) {
                if (!updateKeywordHash(activeDefinition->name)) return false;
            }
        } else {
            return false;  // Unknown opcode
        }
    }

    // V2+ hash verification
    if (mBytecodeVersion >= 2) {
        std::vector<uint8_t> computedHash = getFinalKeywordHash();
        if (computedHash != extractedHash) return false;
    }

    // Build result string
    sourceCode.clear();
    for (const auto& word : resultWords) {
        if (word.empty()) continue;
        if (!sourceCode.empty()) sourceCode += " ";
        sourceCode += word;
    }

    return !sourceCode.empty();
}

#endif // GRIDSCRIPT_STANDALONE_TEST
