#pragma once

#include "core/partition.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace pl {

struct PartitionMetrics {
    bool exact_recovery = false;
    double pairwise_precision = 0.0;
    double pairwise_recall = 0.0;
    double pairwise_f1 = 0.0;
    double adjusted_rand_index = 0.0;
};

namespace detail {
inline long double choose2(std::size_t n) noexcept {
    return static_cast<long double>(n) * static_cast<long double>(n - (n > 0 ? 1 : 0)) / 2.0L;
}
struct LabelPairHash {
    std::size_t operator()(const std::pair<ClusterId, ClusterId>& p) const noexcept {
        const auto a = std::hash<ClusterId>{}(p.first);
        const auto b = std::hash<ClusterId>{}(p.second);
        return a ^ (b + std::size_t{0x9e3779b9} + (a << 6) + (a >> 2));
    }
};
} // namespace detail

// O(n) evaluation without enumerating O(n^2) element pairs.
inline PartitionMetrics evaluate_partition(const Partition& truth,
                                            const Partition& predicted) {
    if (truth.size() != predicted.size())
        throw std::invalid_argument("Partition sizes must match");

    std::unordered_map<ClusterId, std::size_t> truth_sizes;
    std::unordered_map<ClusterId, std::size_t> pred_sizes;
    std::unordered_map<std::pair<ClusterId, ClusterId>, std::size_t,
                       detail::LabelPairHash> contingency;
    for (ElementId i = 0; i < truth.size(); ++i) {
        const auto t = truth.cluster_of(i), p = predicted.cluster_of(i);
        ++truth_sizes[t];
        ++pred_sizes[p];
        ++contingency[{t, p}];
    }

    long double truth_pos = 0, pred_pos = 0, tp = 0;
    for (const auto& [_, count] : truth_sizes) truth_pos += detail::choose2(count);
    for (const auto& [_, count] : pred_sizes) pred_pos += detail::choose2(count);
    for (const auto& [_, count] : contingency) tp += detail::choose2(count);

    PartitionMetrics m;
    m.exact_recovery = (tp == truth_pos && tp == pred_pos);
    // No predicted positive pairs: precision 1 by convention (no false positives).
    // No true positive pairs: recall 1 by convention (no missed positive pairs).
    m.pairwise_precision = pred_pos == 0 ? 1.0 : static_cast<double>(tp / pred_pos);
    m.pairwise_recall = truth_pos == 0 ? 1.0 : static_cast<double>(tp / truth_pos);
    const double sum = m.pairwise_precision + m.pairwise_recall;
    m.pairwise_f1 = sum == 0 ? 0.0 :
        2.0 * m.pairwise_precision * m.pairwise_recall / sum;

    const long double possible = detail::choose2(truth.size());
    if (possible == 0) {
        m.adjusted_rand_index = 1.0;
        return m;
    }
    const long double expected = truth_pos * pred_pos / possible;
    const long double max_index = (truth_pos + pred_pos) / 2.0L;
    const long double denominator = max_index - expected;
    // All-one-cluster or all-singleton degeneracies yield denominator = 0.
    m.adjusted_rand_index = std::abs(denominator) < 1e-18L
        ? (m.exact_recovery ? 1.0 : 0.0)
        : static_cast<double>((tp - expected) / denominator);
    return m;
}

} // namespace pl
