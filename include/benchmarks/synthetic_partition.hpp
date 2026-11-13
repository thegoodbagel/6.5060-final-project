#pragma once

#include "core/partition.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string_view>

namespace pl {

inline Partition make_synthetic_partition(std::size_t n, std::size_t k,
                                          std::uint64_t seed, std::string_view shape,
                                          double exponent = 2.0) {
    if (k == 0 || k > n) throw std::invalid_argument("Require 1 <= k <= n");
    if (!std::isfinite(exponent) || exponent <= 0)
        throw std::invalid_argument("Require a positive finite power-law exponent");

    std::vector<ClusterId> labels;
    labels.reserve(n);
    std::mt19937_64 rng(seed);
    if (shape == "balanced") {
        for (std::size_t i = 0; i < n; ++i) labels.push_back(i % k);
    } else {
        for (ClusterId id = 0; id < k; ++id) labels.push_back(id);
        if (shape == "one-large") {
            labels.resize(n, 0);
        } else if (shape == "singleton-heavy") {
            const auto active = std::max<std::size_t>(1, k / 4);
            while (labels.size() < n) labels.push_back((labels.size() - k) % active);
        } else if (shape == "uniform" || shape == "zipf" || shape == "power-law") {
            std::vector<double> weights(k, 1.0);
            if (shape != "uniform") {
                const double power = shape == "zipf" ? 1.0 : exponent;
                for (ClusterId id = 0; id < k; ++id)
                    weights[id] = std::pow(static_cast<double>(id + 1), -power);
            }
            std::discrete_distribution<ClusterId> cluster(weights.begin(), weights.end());
            while (labels.size() < n) labels.push_back(cluster(rng));
        } else {
            throw std::invalid_argument("Unknown partition shape");
        }
    }
    std::shuffle(labels.begin(), labels.end(), rng);
    return Partition(std::move(labels));
}

} // namespace pl
