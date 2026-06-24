// float_helpers.hpp — shared float test utilities.
#pragma once
#include <cmath>

// Tolerant float comparison (relative for normal, absolute for near-zero).
inline bool feq(float a, float b, float tol = 1e-5f)
{
    if (std::fabs(b) > 1e-10f)
        return std::fabs((a - b) / b) < tol;
    return std::fabs(a - b) < tol;
}
