#pragma once

#include "core/algorithm.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace pl {

// Vesdapunt, Bellare, and Dalvi (2014), Algorithm 5.
class NodeOrdering final : public Algorithm {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "node_ordering";
    }

    [[nodiscard]] AlgorithmResult run(std::size_t n, Oracle& oracle,
                                      Proxy* proxy = nullptr) const override {
        if (n == 0 || proxy == nullptr)
            throw std::invalid_argument("Node Ordering requires elements and a proxy");

        const auto started = std::chrono::steady_clock::now();
        std::vector<std::vector<double>> scores(n);
        std::vector<long double> totals(n, 0);
        for (ElementId b = 0; b < n; ++b) {
            scores[b].reserve(b);
            for (ElementId a = 0; a < b; ++a) {
                const auto score = proxy->score(a, b);
                if (!std::isfinite(score))
                    throw std::invalid_argument("Node Ordering requires finite proxy scores");
                scores[b].push_back(score);
                totals[a] += score;
                totals[b] += score;
            }
        }
        std::vector<ElementId> order(n);
        std::iota(order.begin(), order.end(), ElementId{0});
        std::stable_sort(order.begin(), order.end(), [&totals](auto a, auto b) {
            return totals[a] > totals[b];
        });
        const auto sorted = std::chrono::steady_clock::now();

        std::vector<ClusterId> labels(n);
        std::vector<ElementId> best(n);
        std::vector<ClusterId> candidates;
        candidates.reserve(n);
        std::size_t clusters = 0;
        std::size_t queries = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const auto current = order[i];
            const auto score_to = [&scores, current](ElementId other) {
                return scores[std::max(current, other)][std::min(current, other)];
            };
            // A negative answer eliminates the entire known cluster. Keep
            // its first member in descending score order (ID breaks ties).
            std::fill_n(best.begin(), clusters, n);
            for (std::size_t j = 0; j < i; ++j) {
                const auto other = order[j];
                auto& member = best[labels[other]];
                if (member == n || score_to(other) > score_to(member) ||
                    (score_to(other) == score_to(member) && other < member))
                    member = other;
            }
            candidates.resize(clusters);
            std::iota(candidates.begin(), candidates.end(), ClusterId{0});
            std::sort(candidates.begin(), candidates.end(), [&](auto a, auto b) {
                const auto first = score_to(best[a]);
                const auto second = score_to(best[b]);
                return first == second ? best[a] < best[b] : first > second;
            });
            auto assigned = clusters;
            for (const auto cluster : candidates) {
                ++queries;
                if (oracle.query(current, best[cluster])) {
                    assigned = cluster;
                    break;
                }
            }
            if (assigned == clusters) ++clusters;
            labels[current] = assigned;
        }

        AlgorithmResult result(Partition(std::move(labels)),
                               std::vector<std::size_t>(queries, 1));
        result.preprocessing_ms =
            std::chrono::duration<double, std::milli>(sorted - started).count();
        return result;
    }
};

} // namespace pl
