#pragma once
#include <cstdint>

namespace DeathExperience {
// Native regular EXP is uint32. Keep the complete domain through loss math.
constexpr std::uint32_t Remaining(std::uint32_t current, std::uint32_t loss)
{
    // Preserve the original exhaustion convention, including equality -> 0.
    return loss > current ? 1u : current - loss;
}
}
