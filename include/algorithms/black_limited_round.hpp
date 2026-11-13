#pragma once

#include "core/algorithm.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace pl {

// Black, Mazumdar, and Saha (2025), Algorithm 1, pairwise queries.
class BlackLimitedRound final : public Algorithm {
public:
    BlackLimitedRound(std::size_t cluster_bound, std::size_t rounds)
        : cluster_bound_(cluster_bound), rounds_(rounds) {
        if (cluster_bound == 0 || rounds == 0)
            throw std::invalid_argument("Black limited-round requires positive k bound and rounds");
    }

    [[nodiscard]] std::string_view name() const noexcept override {
        return "black_limited_round";
    }

    [[nodiscard]] AlgorithmResult run(std::size_t n, Oracle& oracle,
                                      Proxy* = nullptr) const override {
        if (n == 0) throw std::invalid_argument("Require at least one element");
        std::vector<ElementId> active(n);
        std::iota(active.begin(), active.end(), ElementId{0});
        auto parent = active;
        const auto root = [&parent](ElementId id) {
            while (id != parent[id]) {
                parent[id] = parent[parent[id]];
                id = parent[id];
            }
            return id;
        };

        std::vector<std::size_t> counts;
        for (auto remaining = rounds_; active.size() > 1; --remaining) {
            const auto size = active.size();
            // ceil(n / 16) <= k is equivalent to n <= 16k without overflow.
            const bool base_case = remaining == 1 ||
                size / 16 + (size % 16 != 0) <= cluster_bound_;
            std::size_t block_size = size;
            if (!base_case) {
                // epsilon = 2^(-r) / (1 - 2^(-r)), avoiding overflow for large r.
                const long double inverse_power =
                    remaining <= std::numeric_limits<long double>::digits
                    ? std::ldexp(1.0L, -static_cast<int>(remaining)) : 0.0L;
                const long double epsilon = inverse_power / (1 - inverse_power);
                const long double ratio = static_cast<long double>(size) / cluster_bound_;
                block_size = static_cast<std::size_t>(std::ceil(
                    3.0L * cluster_bound_ * std::pow(ratio, epsilon)));
            }

            std::size_t queries = 0;
            // active and the block boundaries stay fixed until the round ends.
            // Stream every scheduled pair even if prior answers imply its result.
            for (std::size_t start = 0; start < size;) {
                const auto end = start + std::min(block_size, size - start);
                for (auto i = start; i < end; ++i) {
                    for (auto j = i + 1; j < end; ++j) {
                        ++queries;
                        if (oracle.query(active[i], active[j])) {
                            const auto a = root(active[i]);
                            const auto b = root(active[j]);
                            parent[std::max(a, b)] = std::min(a, b);
                        }
                    }
                }
                start = end;
            }
            counts.push_back(queries);
            if (base_case) break;

            // Each local component contributes its smallest element as representative.
            std::erase_if(active, [&root](ElementId id) { return root(id) != id; });
        }

        std::vector<ClusterId> labels(n);
        for (ElementId id = 0; id < n; ++id) labels[id] = root(id);
        return AlgorithmResult(Partition(std::move(labels)), std::move(counts));
    }

private:
    std::size_t cluster_bound_;
    std::size_t rounds_;
};

} // namespace pl
