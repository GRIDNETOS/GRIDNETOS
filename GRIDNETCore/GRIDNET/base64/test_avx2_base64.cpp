/**
 * Self-contained AVX2 Base64 Test
 * Compile with: cl /EHsc /O2 /arch:AVX2 test_avx2_base64.cpp
 * Or: g++ -O2 -mavx2 -o test_avx2_base64 test_avx2_base64.cpp
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <immintrin.h>

// ============================================================================
// Chromium base64 encoder (reference implementation)
// ============================================================================

static const char e0[256] = {
 'A',  'A',  'A',  'A',  'B',  'B',  'B',  'B',  'C',  'C',
 'C',  'C',  'D',  'D',  'D',  'D',  'E',  'E',  'E',  'E',
 'F',  'F',  'F',  'F',  'G',  'G',  'G',  'G',  'H',  'H',
 'H',  'H',  'I',  'I',  'I',  'I',  'J',  'J',  'J',  'J',
 'K',  'K',  'K',  'K',  'L',  'L',  'L',  'L',  'M',  'M',
 'M',  'M',  'N',  'N',  'N',  'N',  'O',  'O',  'O',  'O',
 'P',  'P',  'P',  'P',  'Q',  'Q',  'Q',  'Q',  'R',  'R',
 'R',  'R',  'S',  'S',  'S',  'S',  'T',  'T',  'T',  'T',
 'U',  'U',  'U',  'U',  'V',  'V',  'V',  'V',  'W',  'W',
 'W',  'W',  'X',  'X',  'X',  'X',  'Y',  'Y',  'Y',  'Y',
 'Z',  'Z',  'Z',  'Z',  'a',  'a',  'a',  'a',  'b',  'b',
 'b',  'b',  'c',  'c',  'c',  'c',  'd',  'd',  'd',  'd',
 'e',  'e',  'e',  'e',  'f',  'f',  'f',  'f',  'g',  'g',
 'g',  'g',  'h',  'h',  'h',  'h',  'i',  'i',  'i',  'i',
 'j',  'j',  'j',  'j',  'k',  'k',  'k',  'k',  'l',  'l',
 'l',  'l',  'm',  'm',  'm',  'm',  'n',  'n',  'n',  'n',
 'o',  'o',  'o',  'o',  'p',  'p',  'p',  'p',  'q',  'q',
 'q',  'q',  'r',  'r',  'r',  'r',  's',  's',  's',  's',
 't',  't',  't',  't',  'u',  'u',  'u',  'u',  'v',  'v',
 'v',  'v',  'w',  'w',  'w',  'w',  'x',  'x',  'x',  'x',
 'y',  'y',  'y',  'y',  'z',  'z',  'z',  'z',  '0',  '0',
 '0',  '0',  '1',  '1',  '1',  '1',  '2',  '2',  '2',  '2',
 '3',  '3',  '3',  '3',  '4',  '4',  '4',  '4',  '5',  '5',
 '5',  '5',  '6',  '6',  '6',  '6',  '7',  '7',  '7',  '7',
 '8',  '8',  '8',  '8',  '9',  '9',  '9',  '9',  '+',  '+',
 '+',  '+',  '/',  '/',  '/',  '/'
};

static const char e1[256] = {
 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',
 'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',
 'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',
 'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',
 'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',
 'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
 '8',  '9',  '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',
 'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',
 'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',
 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',
 'k',  'l',  'm',  'n',  'o',  'p',  'q',  'r',  's',  't',
 'u',  'v',  'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',
 '4',  '5',  '6',  '7',  '8',  '9',  '+',  '/',  'A',  'B',
 'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',
 'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',
 'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',  'e',  'f',
 'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',  'p',
 'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',
 '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
 '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',
 'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',
 'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',
 'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',
 'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',
 'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',
 '6',  '7',  '8',  '9',  '+',  '/'
};

static const char e2[256] = {
 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',
 'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',
 'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',
 'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',
 'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',
 'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
 '8',  '9',  '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',
 'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',
 'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',
 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',
 'k',  'l',  'm',  'n',  'o',  'p',  'q',  'r',  's',  't',
 'u',  'v',  'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',
 '4',  '5',  '6',  '7',  '8',  '9',  '+',  '/',  'A',  'B',
 'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',
 'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',
 'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',  'e',  'f',
 'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',  'p',
 'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',
 '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
 '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',
 'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',
 'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',
 'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',
 'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',
 'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',
 '6',  '7',  '8',  '9',  '+',  '/'
};

#define CHARPAD '='

size_t chromium_base64_encode(char* dest, const char* str, size_t len)
{
    size_t i = 0;
    uint8_t* p = (uint8_t*) dest;
    uint8_t t1, t2, t3;

    if (len > 2) {
        for (; i < len - 2; i += 3) {
            t1 = str[i]; t2 = str[i+1]; t3 = str[i+2];
            *p++ = e0[t1];
            *p++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *p++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
            *p++ = e2[t3];
        }
    }

    switch (len - i) {
    case 0:
        break;
    case 1:
        t1 = str[i];
        *p++ = e0[t1];
        *p++ = e1[(t1 & 0x03) << 4];
        *p++ = CHARPAD;
        *p++ = CHARPAD;
        break;
    default:
        t1 = str[i]; t2 = str[i+1];
        *p++ = e0[t1];
        *p++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
        *p++ = e2[(t2 & 0x0F) << 2];
        *p++ = CHARPAD;
    }

    *p = '\0';
    return p - (uint8_t*)dest;
}

// ============================================================================
// AVX2 base64 encoder
// ============================================================================

static inline __m256i enc_reshuffle(const __m256i input) {
    const __m256i in = _mm256_shuffle_epi8(input, _mm256_set_epi8(
        10, 11,  9, 10,
         7,  8,  6,  7,
         4,  5,  3,  4,
         1,  2,  0,  1,

        14, 15, 13, 14,
        11, 12, 10, 11,
         8,  9,  7,  8,
         5,  6,  4,  5
    ));

    const __m256i t0 = _mm256_and_si256(in, _mm256_set1_epi32(0x0fc0fc00));
    const __m256i t1 = _mm256_mulhi_epu16(t0, _mm256_set1_epi32(0x04000040));

    const __m256i t2 = _mm256_and_si256(in, _mm256_set1_epi32(0x003f03f0));
    const __m256i t3 = _mm256_mullo_epi16(t2, _mm256_set1_epi32(0x01000010));

    return _mm256_or_si256(t1, t3);
}

static inline __m256i enc_translate(const __m256i in) {
    const __m256i lut = _mm256_setr_epi8(
        65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0,
        65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);
    __m256i indices = _mm256_subs_epu8(in, _mm256_set1_epi8(51));
    __m256i mask = _mm256_cmpgt_epi8(in, _mm256_set1_epi8(25));
    indices = _mm256_sub_epi8(indices, mask);
    __m256i out = _mm256_add_epi8(in, _mm256_shuffle_epi8(lut, indices));
    return out;
}

size_t fast_avx2_base64_encode(char* dest, const char* str, size_t len) {
    const char* const dest_orig = dest;
    if(len >= 32 - 4) {
        // first load is masked
        __m256i inputvector = _mm256_maskload_epi32((int const*)(str - 4), _mm256_set_epi32(
            0x80000000,
            0x80000000,
            0x80000000,
            0x80000000,
            0x80000000,
            0x80000000,
            0x80000000,
            0x00000000 // we do not load the first 4 bytes
        ));

        while(true) {
            inputvector = enc_reshuffle(inputvector);
            inputvector = enc_translate(inputvector);
            _mm256_storeu_si256((__m256i *)dest, inputvector);
            str += 24;
            dest += 32;
            len -= 24;
            if(len >= 32) {
                inputvector = _mm256_loadu_si256((__m256i *)(str - 4));
            } else {
                break;
            }
        }
    }
    size_t scalarret = chromium_base64_encode(dest, str, len);
    return (dest - dest_orig) + scalarret;
}

// ============================================================================
// Test harness
// ============================================================================

void printHex(const char* data, size_t len, size_t maxLen = 64) {
    for (size_t i = 0; i < len && i < maxLen; i++) {
        printf("%02X ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len > maxLen) printf("... (%zu more bytes)", len - maxLen);
    printf("\n");
}

bool isValidBase64Char(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
}

bool testSize(size_t testSize) {
    printf("\n--- Testing size: %zu ---\n", testSize);

    // Create deterministic test data
    char* testData = (char*)malloc(testSize + 32); // Extra padding for safety
    if (!testData) {
        printf("ERROR: Failed to allocate test data\n");
        return false;
    }

    // Fill with pattern
    for (size_t i = 0; i < testSize + 32; i++) {
        testData[i] = (char)(i % 256);
    }

    // Allocate output buffers
    size_t outputSize = ((testSize + 2) / 3) * 4 + 64;
    char* chromiumOutput = (char*)malloc(outputSize);
    char* avx2Output = (char*)malloc(outputSize);

    if (!chromiumOutput || !avx2Output) {
        printf("ERROR: Failed to allocate output buffers\n");
        free(testData);
        if (chromiumOutput) free(chromiumOutput);
        if (avx2Output) free(avx2Output);
        return false;
    }

    memset(chromiumOutput, 0xCC, outputSize); // Fill with sentinel
    memset(avx2Output, 0xCC, outputSize);

    // Encode with chromium (reference)
    size_t chromiumLen = chromium_base64_encode(chromiumOutput, testData, testSize);
    printf("Chromium encoded length: %zu\n", chromiumLen);

    // Encode with AVX2
    size_t avx2Len = fast_avx2_base64_encode(avx2Output, testData, testSize);
    printf("AVX2 encoded length: %zu\n", avx2Len);

    // Compare lengths
    if (chromiumLen != avx2Len) {
        printf("ERROR: Length mismatch! Chromium: %zu, AVX2: %zu\n", chromiumLen, avx2Len);
        free(testData);
        free(chromiumOutput);
        free(avx2Output);
        return false;
    }

    // Check for invalid characters in AVX2 output
    printf("Checking AVX2 output for invalid characters...\n");
    bool hasInvalidChars = false;
    for (size_t i = 0; i < avx2Len; i++) {
        if (!isValidBase64Char((unsigned char)avx2Output[i])) {
            printf("  Invalid char at position %zu: 0x%02X\n", i, (unsigned char)avx2Output[i]);
            hasInvalidChars = true;
            if (i > 10) {
                printf("  ... (stopping after 10 errors)\n");
                break;
            }
        }
    }

    if (hasInvalidChars) {
        printf("AVX2 output (hex):\n");
        printHex(avx2Output, avx2Len);
        printf("\nChromium output (hex):\n");
        printHex(chromiumOutput, chromiumLen);
    }

    // Compare content byte by byte
    bool contentMatch = true;
    size_t firstMismatch = 0;
    for (size_t i = 0; i < chromiumLen; i++) {
        if (chromiumOutput[i] != avx2Output[i]) {
            if (contentMatch) {
                firstMismatch = i;
                contentMatch = false;
                printf("First mismatch at position %zu:\n", firstMismatch);
                printf("  Chromium: '%c' (0x%02X)\n", chromiumOutput[i], (unsigned char)chromiumOutput[i]);
                printf("  AVX2:     '%c' (0x%02X)\n", avx2Output[i], (unsigned char)avx2Output[i]);
            }
        }
    }

    if (!contentMatch) {
        printf("\nChromium output (first 64 chars): %.64s\n", chromiumOutput);
        printf("AVX2 output (first 64 chars):     %.64s\n", avx2Output);

        free(testData);
        free(chromiumOutput);
        free(avx2Output);
        return false;
    }

    printf("Size %zu: PASS\n", testSize);

    free(testData);
    free(chromiumOutput);
    free(avx2Output);
    return true;
}

int main() {
    printf("=== AVX2 Base64 Encoding Test ===\n");
    printf("Testing AVX2 availability...\n");

    // Check AVX2 support
#if defined(__AVX2__)
    printf("AVX2 support: Compiled with AVX2\n");
#else
    printf("WARNING: Not compiled with AVX2 support!\n");
#endif

    // Test various sizes
    size_t testSizes[] = { 1, 3, 10, 24, 28, 32, 48, 64, 100, 256, 1000, 10000 };
    int numTests = sizeof(testSizes) / sizeof(testSizes[0]);

    int passed = 0;
    int failed = 0;

    for (int i = 0; i < numTests; i++) {
        if (testSize(testSizes[i])) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("\n=== Summary ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);

    return failed > 0 ? 1 : 0;
}
