#pragma once

#include "core/oracle.hpp"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <utility>

namespace pl {

// Exact oracle owns its private copy of the ground-truth partition.
// Mutex protects counters/seen-pair set for concurrent queries.
class ExactOracle final : public Oracle {
public:
    explicit ExactOracle(Partition ground_truth)
        : ground_truth_(std::move(ground_truth)) {}

    bool query(ElementId a, ElementId b) override {
        // Validate before counting; invalid requests do not count as queries.
        const bool answer = ground_truth_.same_cluster(a, b);
        const auto pair = std::minmax(a, b);
        const Pair key{pair.first, pair.second};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++queries_;
            seen_.insert(key);
        }
        return answer;
    }

    [[nodiscard]] std::uint64_t query_count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return queries_;
    }

    [[nodiscard]] std::uint64_t unique_query_count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<std::uint64_t>(seen_.size());
    }

    void reset_counters() override {
        std::lock_guard<std::mutex> lock(mutex_);
        queries_ = 0;
        seen_.clear();
    }

private:
    using Pair = std::pair<ElementId, ElementId>;

    struct PairHash {
        std::size_t operator()(const Pair& p) const noexcept {
            const auto h1 = std::hash<ElementId>{}(p.first);
            const auto h2 = std::hash<ElementId>{}(p.second);
            return h1 ^ (h2 + std::size_t{0x9e3779b9} + (h1 << 6) + (h1 >> 2));
        }
    };

    const Partition ground_truth_;
    mutable std::mutex mutex_;
    std::uint64_t queries_ = 0;
    std::unordered_set<Pair, PairHash> seen_;
};

} // namespace pl

