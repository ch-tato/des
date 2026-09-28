#pragma once
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace des
{

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

// key schedule
struct KeySchedule
{
    std::array<Bits, 16> roundKeys;
};

inline KeySchedule generateRoundKeys(const Bits &key64, bool verbose)
{
}

} // namespace des