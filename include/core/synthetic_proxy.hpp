#pragma once

#include "core/proxy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace pl {

// Lazy, deterministic symmetric proxy: no global RNG or dense n*n matrix.
// For each unordered pair, sample a normal score with a class-conditional mean.
// The truth stays private, accessible only to this synthetic data generator.
class SyntheticProxy final : public Proxy {
public:
    struct Parameters {
        double within_mean = 0.8;
        double between_mean = 0.2;
        double standard_deviation = 0.1;
        std::uint64_t seed = 0;
        bool cache = false;
    };

    explicit SyntheticProxy(Partition truth)
        : SyntheticProxy(std::move(truth), Parameters{}) {}

    SyntheticProxy(Partition truth, Parameters params)
        : truth_(std::move(truth)), params_(params) {
        if (!std::isfinite(params_.within_mean) ||
            !std::isfinite(params_.between_mean) ||
            !std::isfinite(params_.standard_deviation) ||
            params_.standard_deviation < 0) {
            throw std::invalid_argument("Proxy parameters must be finite; sigma >= 0");
        }
    }

    double score(ElementId a, ElementId b) override {
        // Reject invalid IDs before incrementing counts.
        const bool same = truth_.same_cluster(a, b);
        const auto key = std::minmax(a, b);
        const Pair pair{key.first, key.second};
        std::lock_guard<std::mutex> lock(mutex_);
        ++accesses_;
        if (params_.cache) {
            auto it = cache_.find(pair);
            if (it != cache_.end()) return it->second;
        }
        const double value = calculate(pair, same);
        ++computations_;
        if (params_.cache) cache_.emplace(pair, value);
        return value;
    }

    [[nodiscard]] std::uint64_t access_count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return accesses_;
    }

    [[nodiscard]] std::uint64_t computation_count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return computations_;
    }

    void reset_counters() override {
        std::lock_guard<std::mutex> lock(mutex_);
        accesses_ = 0;
        computations_ = 0;
        // Preserve memoization. Counters reflect work after reset, not cold-cache cost.
    }

    void clear_cache() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

private:
    using Pair = std::pair<ElementId, ElementId>;

    struct PairHash {
        std::size_t operator()(const Pair& p) const noexcept {
            const auto a = std::hash<ElementId>{}(p.first);
            const auto b = std::hash<ElementId>{}(p.second);
            return a ^ (b + std::size_t{0x9e3779b9} + (a << 6) + (a >> 2));
        }
    };

    static std::uint64_t mix(std::uint64_t x) noexcept {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    static double unit_open(std::uint64_t bits) noexcept {
        // Strictly between 0 and 1, so log() in Box-Muller is defined.
        return (static_cast<double>(bits >> 11) + 0.5) * 0x1.0p-53;
    }

    [[nodiscard]] double calculate(Pair pair, bool same) const {
        const auto h1 = mix(params_.seed ^ mix(static_cast<std::uint64_t>(pair.first)));
        const auto h2 = mix(h1 ^ mix(static_cast<std::uint64_t>(pair.second)));
        const double u1 = unit_open(mix(h2));
        const double u2 = unit_open(mix(h2 ^ 0x72616e646f6dULL));
        constexpr double two_pi = 6.283185307179586476925286766559;
        const double z = std::sqrt(-2.0 * std::log(u1)) * std::cos(two_pi * u2);
        return (same ? params_.within_mean : params_.between_mean)
            + params_.standard_deviation * z;
    }

    const Partition truth_;
    const Parameters params_;
    mutable std::mutex mutex_;
    std::uint64_t accesses_ = 0;
    std::uint64_t computations_ = 0;
    std::unordered_map<Pair, double, PairHash> cache_;
};

} // namespace pl

