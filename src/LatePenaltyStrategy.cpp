#include "LatePenaltyStrategy.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

double StandardLatePenalty::penalty(double maximumMark, int daysLate) const {
    if (!std::isfinite(maximumMark) || maximumMark < 0.0 || daysLate < 0) {
        throw std::invalid_argument("Maximum mark and days late must be non-negative.");
    }
    return std::min(daysLate, 3) * maximumMark * 0.10;
}

double StandardLatePenalty::finalMark(double rawMark, double maximumMark, int daysLate) const {
    if (!std::isfinite(rawMark) || rawMark < 0.0 || rawMark > maximumMark) {
        throw std::invalid_argument("Raw mark must be between zero and the assignment maximum.");
    }
    if (daysLate > 3) return 0.0;
    return std::max(0.0, rawMark - penalty(maximumMark, daysLate));
}
