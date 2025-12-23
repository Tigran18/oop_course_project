#pragma once
#include <algorithm>
#include <cmath>

// Fits (w,h) into (maxW,maxH) preserving aspect ratio.
// If (w,h) already fit, nothing changes.
// Guarantees w>=1 and h>=1 if input are positive.
inline void fitKeepAspect(int& w, int& h, int maxW, int maxH)
{
    if (w <= 0 || h <= 0) {
        w = std::max(1, w);
        h = std::max(1, h);
        return;
    }
    if (maxW <= 0 || maxH <= 0) return;

    if (w <= maxW && h <= maxH) return;

    const double sx = static_cast<double>(maxW) / static_cast<double>(w);
    const double sy = static_cast<double>(maxH) / static_cast<double>(h);
    const double s  = std::min(sx, sy);

    w = std::max(1, static_cast<int>(std::llround(w * s)));
    h = std::max(1, static_cast<int>(std::llround(h * s)));
}
