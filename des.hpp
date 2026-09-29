#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace des
{

// standard des tables
static const int PC1_TABLE[56] = {57, 49, 41, 33, 25, 17, 9,  1,  58, 50, 42, 34, 26, 18, 10, 2,  59, 51, 43,
                                  35, 27, 19, 11, 3,  60, 52, 44, 36, 63, 55, 47, 39, 31, 23, 15, 7,  62, 54,
                                  46, 38, 30, 22, 14, 6,  61, 53, 45, 37, 29, 21, 13, 5,  28, 20, 12, 4};

static const int PC2_TABLE[48] = {14, 17, 11, 24, 1,  5,  3,  28, 15, 6,  21, 10, 23, 19, 12, 4,
                                  26, 8,  16, 7,  27, 20, 13, 2,  41, 52, 31, 37, 47, 55, 30, 40,
                                  51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32};

static const int SHIFT_SCHEDULE[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

// bit utilities
using Bits = std::vector<int>; // 0 or 1

inline Bits bytesToBits(const std::vector<uint8_t> &bytes)
{
    Bits bits;
    bits.reserve(bytes.size() * 8);
    for (uint8_t b : bytes)
        for (int i = 7; i >= 0; --i)
            bits.push_back((b >> i) & 1);
    return bits;
}

inline std::string bitsToHex(const Bits &bits)
{
    std::ostringstream oss;
    for (size_t i = 0; i < bits.size(); i += 4)
    {
        int val = 0;
        for (int j = 0; j < 4; j++)
            val = (val << 1) | bits[i + j];
        oss << std::hex << val;
    }
    return oss.str();
}

inline Bits permute(const Bits &input, const int *table, int outSize)
{
    Bits out(outSize);
    for (int i = 0; i < outSize; i++)
        out[i] = input[table[i] - 1];
    return out;
}

inline Bits leftRotate(const Bits &half, int n)
{
    Bits out(half.size());
    int size = static_cast<int>(half.size());
    for (int i = 0; i < size; i++)
        out[i] = half[(i + n) % size];
    return out;
}

// key schedule
struct KeySchedule
{
    std::array<Bits, 16> roundKeys;
};

inline KeySchedule generateRoundKeys(const Bits &key64, bool verbose)
{
    KeySchedule ks;
    Bits key56 = permute(key64, PC1_TABLE, 56);
    Bits C(key56.begin(), key56.begin() + 28);
    Bits D(key56.begin() + 28, key56.end());

    if (verbose)
        std::cout << "  [KeySchedule] PC-1 -> 56 bits. C0=" << bitsToHex(C) << " D0=" << bitsToHex(D) << "\n";

    for (int round = 0; round < 16; round++)
    {
        C = leftRotate(C, SHIFT_SCHEDULE[round]);
        D = leftRotate(D, SHIFT_SCHEDULE[round]);
        Bits CD;
        CD.reserve(56);
        CD.insert(CD.end(), C.begin(), C.end());
        Bits roundKey = permute(CD, PC2_TABLE, 48);
        ks.roundKeys[round] = roundKey;
        if (verbose)
            std::cout << "  [KeySchedule] Round " << (round + 1) << " (shift " << SHIFT_SCHEDULE[round]
                      << "): C=" << bitsToHex(C) << " D=" << bitsToHex(D) << " -> K" << (round + 1) << "="
                      << bitsToHex(roundKey) << "\n";
    }
    return ks;
}

} // namespace des