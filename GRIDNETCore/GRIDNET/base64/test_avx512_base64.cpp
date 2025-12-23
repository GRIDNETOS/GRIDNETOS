/**
 * Self-contained AVX512 Base64 Test
 * Compile with: cl /EHsc /O2 /arch:AVX512 test_avx512_base64.cpp
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
// AVX512 base64 encoder (EXACT copy from fastavx512bwbase64.cpp)
// ============================================================================

static inline __m512i enc_reshuffle(const __m512i input) {
    // from https://github.com/WojciechMula/base64simd/blob/master/encode/encode.avx512bw.cpp
    // place each 12-byte subarray in separate 128-bit lane
    const __m512i tmp1 = _mm512_permutexvar_epi32(
        _mm512_set_epi32(-1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0),
        input
    );

    // reshuffle bytes within 128-bit lanes to format required by
    // AVX512BW unpack procedure
    const __m512i in = _mm512_shuffle_epi8(
        tmp1,
        _mm512_set4_epi32(0x0a0b090a, 0x07080607, 0x04050304, 0x01020001)
    );

    // FIX: Extract 6-bit values using mask and multiply (was MISSING in original!)
    // This is the critical step that extracts 4 x 6-bit values from each 3-byte group
    const __m512i t0 = _mm512_and_si512(in, _mm512_set1_epi32(0x0fc0fc00));
    const __m512i t1 = _mm512_mulhi_epu16(t0, _mm512_set1_epi32(0x04000040));

    const __m512i t2 = _mm512_and_si512(in, _mm512_set1_epi32(0x003f03f0));
    const __m512i t3 = _mm512_mullo_epi16(t2, _mm512_set1_epi32(0x01000010));

    return _mm512_or_si512(t1, t3);
}

static inline __m512i enc_translate(const __m512i input) {
    // from https://github.com/WojciechMula/base64simd/blob/master/encode/lookup.avx512bw.cpp
    __m512i result = _mm512_subs_epu8(input, _mm512_set1_epi8(51));

    // distinguish between ranges 0..25 and 26..51
    const __mmask64 less = _mm512_cmpgt_epi8_mask(_mm512_set1_epi8(26), input);
    result = _mm512_mask_mov_epi8(result, less, _mm512_set1_epi8(13));

    const __m512i shift_LUT = _mm512_set4_epi32(
        0x000041f0,
        0xedfcfcfc,
        0xfcfcfcfc,
        0xfcfcfc47
    );

    result = _mm512_shuffle_epi8(shift_LUT, result);

    return _mm512_add_epi8(result, input);
}

size_t fast_avx512bw_base64_encode(char* dest, const char* str, size_t len) {
    const char* const dest_orig = dest;
    __m512i inputvector;

    while (len >= 64) {
        inputvector = _mm512_loadu_si512((__m512i *)(str));  // Note: reads from str, NOT str-4
        inputvector = enc_reshuffle(inputvector);
        inputvector = enc_translate(inputvector);
        _mm512_storeu_si512((__m512i *)dest, inputvector);
        str  += 48;
        dest += 64;
        len -= 48;
    }

    size_t scalarret = chromium_base64_encode(dest, str, len);
    return (dest - dest_orig) + scalarret;
}

// ============================================================================
// Chromium base64 decoder (reference implementation)
// ============================================================================

#define MODP_B64_ERROR ((size_t)-1)
#define BADCHAR 0x01FFFFFF

static const uint32_t d0[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x000000f8, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x000000fc,
0x000000d0, 0x000000d4, 0x000000d8, 0x000000dc, 0x000000e0, 0x000000e4,
0x000000e8, 0x000000ec, 0x000000f0, 0x000000f4, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00000004, 0x00000008, 0x0000000c, 0x00000010, 0x00000014, 0x00000018,
0x0000001c, 0x00000020, 0x00000024, 0x00000028, 0x0000002c, 0x00000030,
0x00000034, 0x00000038, 0x0000003c, 0x00000040, 0x00000044, 0x00000048,
0x0000004c, 0x00000050, 0x00000054, 0x00000058, 0x0000005c, 0x00000060,
0x00000064, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x00000068, 0x0000006c, 0x00000070, 0x00000074, 0x00000078,
0x0000007c, 0x00000080, 0x00000084, 0x00000088, 0x0000008c, 0x00000090,
0x00000094, 0x00000098, 0x0000009c, 0x000000a0, 0x000000a4, 0x000000a8,
0x000000ac, 0x000000b0, 0x000000b4, 0x000000b8, 0x000000bc, 0x000000c0,
0x000000c4, 0x000000c8, 0x000000cc, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};

static const uint32_t d1[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x0000e003, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x0000f003,
0x00004003, 0x00005003, 0x00006003, 0x00007003, 0x00008003, 0x00009003,
0x0000a003, 0x0000b003, 0x0000c003, 0x0000d003, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00001000, 0x00002000, 0x00003000, 0x00004000, 0x00005000, 0x00006000,
0x00007000, 0x00008000, 0x00009000, 0x0000a000, 0x0000b000, 0x0000c000,
0x0000d000, 0x0000e000, 0x0000f000, 0x00000001, 0x00001001, 0x00002001,
0x00003001, 0x00004001, 0x00005001, 0x00006001, 0x00007001, 0x00008001,
0x00009001, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x0000a001, 0x0000b001, 0x0000c001, 0x0000d001, 0x0000e001,
0x0000f001, 0x00000002, 0x00001002, 0x00002002, 0x00003002, 0x00004002,
0x00005002, 0x00006002, 0x00007002, 0x00008002, 0x00009002, 0x0000a002,
0x0000b002, 0x0000c002, 0x0000d002, 0x0000e002, 0x0000f002, 0x00000003,
0x00001003, 0x00002003, 0x00003003, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};

static const uint32_t d2[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x00800f00, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00c00f00,
0x00000d00, 0x00400d00, 0x00800d00, 0x00c00d00, 0x00000e00, 0x00400e00,
0x00800e00, 0x00c00e00, 0x00000f00, 0x00400f00, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00400000, 0x00800000, 0x00c00000, 0x00000100, 0x00400100, 0x00800100,
0x00c00100, 0x00000200, 0x00400200, 0x00800200, 0x00c00200, 0x00000300,
0x00400300, 0x00800300, 0x00c00300, 0x00000400, 0x00400400, 0x00800400,
0x00c00400, 0x00000500, 0x00400500, 0x00800500, 0x00c00500, 0x00000600,
0x00400600, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x00800600, 0x00c00600, 0x00000700, 0x00400700, 0x00800700,
0x00c00700, 0x00000800, 0x00400800, 0x00800800, 0x00c00800, 0x00000900,
0x00400900, 0x00800900, 0x00c00900, 0x00000a00, 0x00400a00, 0x00800a00,
0x00c00a00, 0x00000b00, 0x00400b00, 0x00800b00, 0x00c00b00, 0x00000c00,
0x00400c00, 0x00800c00, 0x00c00c00, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};

static const uint32_t d3[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x003e0000, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x003f0000,
0x00340000, 0x00350000, 0x00360000, 0x00370000, 0x00380000, 0x00390000,
0x003a0000, 0x003b0000, 0x003c0000, 0x003d0000, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00010000, 0x00020000, 0x00030000, 0x00040000, 0x00050000, 0x00060000,
0x00070000, 0x00080000, 0x00090000, 0x000a0000, 0x000b0000, 0x000c0000,
0x000d0000, 0x000e0000, 0x000f0000, 0x00100000, 0x00110000, 0x00120000,
0x00130000, 0x00140000, 0x00150000, 0x00160000, 0x00170000, 0x00180000,
0x00190000, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x001a0000, 0x001b0000, 0x001c0000, 0x001d0000, 0x001e0000,
0x001f0000, 0x00200000, 0x00210000, 0x00220000, 0x00230000, 0x00240000,
0x00250000, 0x00260000, 0x00270000, 0x00280000, 0x00290000, 0x002a0000,
0x002b0000, 0x002c0000, 0x002d0000, 0x002e0000, 0x002f0000, 0x00300000,
0x00310000, 0x00320000, 0x00330000, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};

size_t chromium_base64_decode(char* dest, const char* src, size_t len)
{
    if (len == 0) return 0;

    // if padding is used, then the message must be at least 4 chars and be a multiple of 4
    if (len < 4 || (len % 4 != 0)) {
      return MODP_B64_ERROR;
    }
    // there can be at most 2 pad chars at the end
    if (src[len-1] == CHARPAD) {
        len--;
        if (src[len -1] == CHARPAD) {
            len--;
        }
    }

    size_t i;
    int leftover = len % 4;
    size_t chunks = (leftover == 0) ? len / 4 - 1 : len /4;

    uint8_t* p = (uint8_t*)dest;
    uint32_t x = 0;
    const uint8_t* y = (uint8_t*)src;
    for (i = 0; i < chunks; ++i, y += 4) {
        x = d0[y[0]] | d1[y[1]] | d2[y[2]] | d3[y[3]];
        if (x >= BADCHAR) return MODP_B64_ERROR;
        *p++ =  ((uint8_t*)(&x))[0];
        *p++ =  ((uint8_t*)(&x))[1];
        *p++ =  ((uint8_t*)(&x))[2];
    }

    switch (leftover) {
    case 0:
        x = d0[y[0]] | d1[y[1]] | d2[y[2]] | d3[y[3]];
        if (x >= BADCHAR) return MODP_B64_ERROR;
        *p++ =  ((uint8_t*)(&x))[0];
        *p++ =  ((uint8_t*)(&x))[1];
        *p =    ((uint8_t*)(&x))[2];
        return (chunks+1)*3;
    case 1:
        x = d0[y[0]];
        *p = *((uint8_t*)(&x));
        break;
    case 2:
        x = d0[y[0]] | d1[y[1]];
        *p = *((uint8_t*)(&x));
        break;
    default:
        x = d0[y[0]] | d1[y[1]] | d2[y[2]];
        *p++ =  ((uint8_t*)(&x))[0];
        *p =  ((uint8_t*)(&x))[1];
        break;
    }

    if (x >= BADCHAR) return MODP_B64_ERROR;

    return 3*chunks + (6*leftover)/8;
}

// ============================================================================
// AVX512 base64 decoder (with padding edge case fix)
// ============================================================================

static inline __m512i dec_reshuffle(__m512i input) {
    const __m512i merge_ab_and_bc = _mm512_maddubs_epi16(input, _mm512_set1_epi32(0x01400140));
    return _mm512_madd_epi16(merge_ab_and_bc, _mm512_set1_epi32(0x00011000));
}

#define build_dword(b0, b1, b2, b3) \
     (((uint32_t)((uint8_t)(b0)) << 0*8) \
    | ((uint32_t)((uint8_t)(b1)) << 1*8) \
    | ((uint32_t)((uint8_t)(b2)) << 2*8) \
    | ((uint32_t)((uint8_t)(b3)) << 3*8))

#define _mm512_set4lanes_epi8(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15) \
    _mm512_setr4_epi32( \
        build_dword( b0,  b1,  b2,  b3), \
        build_dword( b4,  b5,  b6,  b7), \
        build_dword( b8,  b9, b10, b11), \
        build_dword(b12, b13, b14, b15))

size_t fast_avx512bw_base64_decode(char *out, const char *src, size_t srclen) {
  char* out_orig = out;

  // FIX: If input ends with padding ('='), leave the padded block for scalar decoder
  // AVX512 cannot handle padding characters - they fail validation
  size_t avx_processable = srclen;
  if (srclen >= 64 && (src[srclen-1] == '=' || (srclen > 1 && src[srclen-2] == '='))) {
    // Round down to previous 64-byte boundary, leaving padded block for scalar
    avx_processable = (srclen - 1) & ~(size_t)63;
  }

  while (avx_processable >= 64) {

    // load
    const __m512i input = _mm512_loadu_si512((const __m512i*)(src));

    // translate from ASCII
    const __m512i higher_nibble = _mm512_and_si512(_mm512_srli_epi32(input, 4), _mm512_set1_epi8(0x0f));
    const __m512i lower_nibble  = _mm512_and_si512(input, _mm512_set1_epi8(0x0f));

    const __m512i shiftLUT = _mm512_set4lanes_epi8(
        0,   0,  19,   4, -65, -65, -71, -71,
        0,   0,   0,   0,   0,   0,   0,   0);

    const __m512i maskLUT  = _mm512_set4lanes_epi8(
        0xa8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8,
        0xf8, 0xf8, 0xf0, 0x54, 0x50, 0x50, 0x50, 0x54);

    const __m512i bitposLUT = _mm512_set4lanes_epi8(
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    const __m512i   sh      = _mm512_shuffle_epi8(shiftLUT,  higher_nibble);
    const __mmask64 eq_2f   = _mm512_cmpeq_epi8_mask(input, _mm512_set1_epi8(0x2f));
    const __m512i   shift   = _mm512_mask_mov_epi8(sh, eq_2f, _mm512_set1_epi8(16));

    const __m512i M         = _mm512_shuffle_epi8(maskLUT,   lower_nibble);
    const __m512i bit       = _mm512_shuffle_epi8(bitposLUT, higher_nibble);

    const uint64_t match    = _mm512_test_epi8_mask(M, bit);

    if (match != (uint64_t)(-1)) {
        return MODP_B64_ERROR;
    }

    const __m512i translated = _mm512_add_epi8(input, shift);
    const __m512i packed = dec_reshuffle(translated);

    const __m512i t1 = _mm512_shuffle_epi8(
        packed,
        _mm512_set4lanes_epi8(
             2,  1,  0,
             6,  5,  4,
            10,  9,  8,
            14, 13, 12,
            -1, -1, -1, -1)
    );

    const __m512i s6 = _mm512_setr_epi32(
         0,  1,  2,
         4,  5,  6,
         8,  9, 10,
        12, 13, 14,
         0,  0,  0, 0);

    const __m512i t2 = _mm512_permutexvar_epi32(s6, t1);

    _mm512_storeu_si512((__m512i*)(out), t2);

    avx_processable -= 64;
    srclen -= 64;
    src += 64;
    out += 48;
  }
  size_t scalarret = chromium_base64_decode(out, src, srclen);
  if(scalarret == MODP_B64_ERROR) return MODP_B64_ERROR;
  return (out - out_orig) + scalarret;
}

// ============================================================================
// Test harness
// ============================================================================

bool isValidBase64Char(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
}

bool testSize(size_t testSize) {
    printf("\n--- Testing size: %zu ---\n", testSize);

    // Create test data with 4 bytes padding BEFORE the start
    // This is important because AVX512 reads from (str - 4)
    char* testDataBase = (char*)_aligned_malloc(testSize + 64, 64);
    if (!testDataBase) {
        printf("ERROR: Failed to allocate test data\n");
        return false;
    }

    char* testData = testDataBase + 4; // Leave 4 bytes before

    // Fill with pattern
    for (size_t i = 0; i < testSize; i++) {
        testData[i] = (char)(i % 256);
    }
    // Fill the 4 bytes before with zeros (for safety)
    for (int i = 0; i < 4; i++) {
        testDataBase[i] = 0;
    }

    // Allocate output buffers
    size_t outputSize = ((testSize + 2) / 3) * 4 + 64;
    char* chromiumOutput = (char*)_aligned_malloc(outputSize, 64);
    char* avx512Output = (char*)_aligned_malloc(outputSize, 64);

    if (!chromiumOutput || !avx512Output) {
        printf("ERROR: Failed to allocate output buffers\n");
        _aligned_free(testDataBase);
        if (chromiumOutput) _aligned_free(chromiumOutput);
        if (avx512Output) _aligned_free(avx512Output);
        return false;
    }

    memset(chromiumOutput, 0xCC, outputSize);
    memset(avx512Output, 0xCC, outputSize);

    // Encode with chromium (reference)
    size_t chromiumLen = chromium_base64_encode(chromiumOutput, testData, testSize);
    printf("Chromium encoded length: %zu\n", chromiumLen);

    // Encode with AVX512
    size_t avx512Len = fast_avx512bw_base64_encode(avx512Output, testData, testSize);
    printf("AVX512 encoded length: %zu\n", avx512Len);

    // Compare lengths
    if (chromiumLen != avx512Len) {
        printf("ERROR: Length mismatch! Chromium: %zu, AVX512: %zu\n", chromiumLen, avx512Len);
        _aligned_free(testDataBase);
        _aligned_free(chromiumOutput);
        _aligned_free(avx512Output);
        return false;
    }

    // Check for invalid characters in AVX512 output
    printf("Checking AVX512 output for invalid characters...\n");
    bool hasInvalidChars = false;
    for (size_t i = 0; i < avx512Len; i++) {
        if (!isValidBase64Char((unsigned char)avx512Output[i])) {
            printf("  Invalid char at position %zu: 0x%02X\n", i, (unsigned char)avx512Output[i]);
            hasInvalidChars = true;
            if (i > 10) {
                printf("  ... (stopping after 10 errors)\n");
                break;
            }
        }
    }

    // Compare content byte by byte
    bool contentMatch = true;
    size_t firstMismatch = 0;
    for (size_t i = 0; i < chromiumLen; i++) {
        if (chromiumOutput[i] != avx512Output[i]) {
            if (contentMatch) {
                firstMismatch = i;
                contentMatch = false;
                printf("First mismatch at position %zu:\n", firstMismatch);
                printf("  Chromium: '%c' (0x%02X)\n", chromiumOutput[i], (unsigned char)chromiumOutput[i]);
                printf("  AVX512:   '%c' (0x%02X)\n", avx512Output[i], (unsigned char)avx512Output[i]);
            }
        }
    }

    if (!contentMatch || hasInvalidChars) {
        printf("\nChromium output (first 64 chars): %.64s\n", chromiumOutput);
        printf("AVX512 output (first 64 chars):   %.64s\n", avx512Output);

        _aligned_free(testDataBase);
        _aligned_free(chromiumOutput);
        _aligned_free(avx512Output);
        return false;
    }

    printf("Size %zu: PASS\n", testSize);

    _aligned_free(testDataBase);
    _aligned_free(chromiumOutput);
    _aligned_free(avx512Output);
    return true;
}

// ============================================================================
// Decode roundtrip test - specifically tests 64-byte edge case
// ============================================================================

bool testDecodeRoundtrip(size_t binarySize) {
    printf("\n--- Decode roundtrip test: %zu bytes binary ---\n", binarySize);

    // Allocate binary data
    char* binaryData = (char*)_aligned_malloc(binarySize + 64, 64);
    if (!binaryData) {
        printf("ERROR: Failed to allocate binary data\n");
        return false;
    }

    // Fill with pattern
    for (size_t i = 0; i < binarySize; i++) {
        binaryData[i] = (char)(i % 256);
    }

    // Calculate base64 size
    size_t base64Size = ((binarySize + 2) / 3) * 4;
    printf("Base64 size: %zu bytes\n", base64Size);

    // Allocate base64 buffer
    char* base64Data = (char*)_aligned_malloc(base64Size + 64, 64);
    if (!base64Data) {
        printf("ERROR: Failed to allocate base64 buffer\n");
        _aligned_free(binaryData);
        return false;
    }

    // Encode with chromium
    size_t encodedLen = chromium_base64_encode(base64Data, binaryData, binarySize);
    printf("Encoded length: %zu\n", encodedLen);

    if (encodedLen != base64Size) {
        printf("ERROR: Encoded length mismatch! Expected: %zu, Got: %zu\n", base64Size, encodedLen);
        _aligned_free(binaryData);
        _aligned_free(base64Data);
        return false;
    }

    // Show last few characters to see padding
    printf("Last 4 chars: [%c%c%c%c]\n",
           base64Data[encodedLen-4], base64Data[encodedLen-3],
           base64Data[encodedLen-2], base64Data[encodedLen-1]);

    // Allocate decode buffers
    char* chromiumDecoded = (char*)_aligned_malloc(binarySize + 64, 64);
    char* avx512Decoded = (char*)_aligned_malloc(binarySize + 64, 64);

    if (!chromiumDecoded || !avx512Decoded) {
        printf("ERROR: Failed to allocate decode buffers\n");
        _aligned_free(binaryData);
        _aligned_free(base64Data);
        if (chromiumDecoded) _aligned_free(chromiumDecoded);
        if (avx512Decoded) _aligned_free(avx512Decoded);
        return false;
    }

    memset(chromiumDecoded, 0xCC, binarySize + 64);
    memset(avx512Decoded, 0xCC, binarySize + 64);

    // Decode with chromium (reference)
    size_t chromiumDecodedLen = chromium_base64_decode(chromiumDecoded, base64Data, encodedLen);
    printf("Chromium decoded length: %zu\n", chromiumDecodedLen);

    if (chromiumDecodedLen == MODP_B64_ERROR) {
        printf("ERROR: Chromium decode failed!\n");
        _aligned_free(binaryData);
        _aligned_free(base64Data);
        _aligned_free(chromiumDecoded);
        _aligned_free(avx512Decoded);
        return false;
    }

    // Decode with AVX512
    size_t avx512DecodedLen = fast_avx512bw_base64_decode(avx512Decoded, base64Data, encodedLen);
    printf("AVX512 decoded length: %zu\n", avx512DecodedLen);

    if (avx512DecodedLen == MODP_B64_ERROR) {
        printf("ERROR: AVX512 decode failed! (This is the 64-byte edge case bug)\n");
        _aligned_free(binaryData);
        _aligned_free(base64Data);
        _aligned_free(chromiumDecoded);
        _aligned_free(avx512Decoded);
        return false;
    }

    // Compare lengths
    if (chromiumDecodedLen != avx512DecodedLen) {
        printf("ERROR: Decoded length mismatch! Chromium: %zu, AVX512: %zu\n",
               chromiumDecodedLen, avx512DecodedLen);
        _aligned_free(binaryData);
        _aligned_free(base64Data);
        _aligned_free(chromiumDecoded);
        _aligned_free(avx512Decoded);
        return false;
    }

    // Compare with original
    if (chromiumDecodedLen != binarySize) {
        printf("ERROR: Decoded size doesn't match original! Original: %zu, Decoded: %zu\n",
               binarySize, chromiumDecodedLen);
        _aligned_free(binaryData);
        _aligned_free(base64Data);
        _aligned_free(chromiumDecoded);
        _aligned_free(avx512Decoded);
        return false;
    }

    // Compare chromium decoded with original
    bool chromiumMatch = true;
    for (size_t i = 0; i < binarySize; i++) {
        if (chromiumDecoded[i] != binaryData[i]) {
            printf("Chromium mismatch at position %zu\n", i);
            chromiumMatch = false;
            break;
        }
    }

    // Compare AVX512 decoded with original
    bool avx512Match = true;
    for (size_t i = 0; i < binarySize; i++) {
        if (avx512Decoded[i] != binaryData[i]) {
            printf("AVX512 mismatch at position %zu: expected 0x%02X, got 0x%02X\n",
                   i, (unsigned char)binaryData[i], (unsigned char)avx512Decoded[i]);
            avx512Match = false;
            break;
        }
    }

    _aligned_free(binaryData);
    _aligned_free(base64Data);
    _aligned_free(chromiumDecoded);
    _aligned_free(avx512Decoded);

    if (!chromiumMatch || !avx512Match) {
        printf("FAIL: Decode mismatch\n");
        return false;
    }

    printf("Decode roundtrip %zu bytes: PASS\n", binarySize);
    return true;
}

int main() {
    printf("=== AVX512 Base64 Encoding Test ===\n");

    // Test various sizes
    size_t testSizes[] = { 1, 3, 10, 24, 48, 64, 100, 256, 1000, 10000, 50000 };
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

    printf("\n=== AVX512 Base64 Decoding Test ===\n");

    // Test decode with specific sizes that produce 64-byte base64 output
    // 48 bytes binary -> 64 chars base64 (no padding)
    // 46 bytes binary -> 64 chars base64 (with == padding)
    // 47 bytes binary -> 64 chars base64 (with = padding)
    size_t decodeSizes[] = {
        46,   // 64 chars with == padding (CRITICAL EDGE CASE)
        47,   // 64 chars with = padding (CRITICAL EDGE CASE)
        48,   // 64 chars no padding
        45,   // 60 chars
        49,   // 68 chars
        93,   // 124 chars
        94,   // 128 chars with == padding
        95,   // 128 chars with = padding
        96,   // 128 chars no padding
        100,  // larger
        256,
        1000
    };
    int numDecodeTests = sizeof(decodeSizes) / sizeof(decodeSizes[0]);

    for (int i = 0; i < numDecodeTests; i++) {
        if (testDecodeRoundtrip(decodeSizes[i])) {
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
