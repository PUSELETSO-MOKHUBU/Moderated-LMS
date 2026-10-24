#pragma once

class LatePenaltyStrategy {
public:
    virtual ~LatePenaltyStrategy() = default;
    virtual double penalty(double maximumMark, int daysLate) const = 0;
};

class StandardLatePenalty final : public LatePenaltyStrategy {
public:
    double penalty(double maximumMark, int daysLate) const override;
    double finalMark(double rawMark, double maximumMark, int daysLate) const;
};
