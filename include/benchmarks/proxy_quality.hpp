#pragma once

#include "core/score_distribution.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace pl {

// Estimate overlap and affinity under the equally weighted mixture.
// AUC is the fraction of all sampled positive-negative pairs in score order.
inline void report_proxy_quality(const ScoreDistribution& within,
                                 const ScoreDistribution& between,
                                 std::size_t count, std::uint64_t seed,
                                 std::ostream& out) {
    if (count == 0) throw std::invalid_argument("Require at least one quality sample per class");
    within.validate();
    between.validate();
    std::mt19937_64 rng(seed);
    long double affinity = 0, overlap = 0, wins = 0;
    std::vector<double> positive, negative;
    positive.reserve(count);
    negative.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        positive.push_back(within.sample(rng));
        negative.push_back(between.sample(rng));
        for (double score : {positive.back(), negative.back()}) {
            const double difference = within.log_density(score) - between.log_density(score);
            if (!std::isfinite(score) || std::isnan(difference))
                throw std::runtime_error("Score parameters exceed the numeric range of quality estimation");
            const double ratio = std::exp(-std::abs(difference));
            affinity += 2 * std::sqrt(ratio) / (1 + ratio);
            overlap += 2 * ratio / (1 + ratio);
        }
    }
    std::sort(negative.begin(), negative.end());
    for (const double score : positive) {
        const auto [low, high] = std::equal_range(negative.begin(), negative.end(), score);
        wins += (low - negative.begin()) + 0.5L * (high - low);
    }
    out << std::setprecision(8)
        << "Squared Hellinger estimate: " << 1 - affinity / (2.L * count)
        << "\nOverlap estimate: " << overlap / (2.L * count)
        << "\nAUC estimate: " << wins / count / count << '\n';
    const auto moments = [count, &out](const char* label, const auto& scores) {
        long double mean = 0, m2 = 0;
        std::size_t seen = 0;
        for (const double x : scores) {
            const long double delta = x - mean;
            mean += delta / ++seen;
            m2 += delta * (x - mean);
        }
        out << label << " sample mean: " << mean << ", variance: " << m2 / count << '\n';
    };
    moments("Within", positive);
    moments("Between", negative);
}

} // namespace pl
