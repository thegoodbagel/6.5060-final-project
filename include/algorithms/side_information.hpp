#pragma once

#include "core/algorithm.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <numeric>
#include <random>
#include <stdexcept>

namespace pl {

// Monte Carlo growth and membership phases of Mazumdar and Saha (2017).
// Fixed bins adapt the paper's finite alphabet to real-valued proxy scores.
class SideInformation final : public Algorithm {
public:
    explicit SideInformation(std::uint64_t seed = 42) : seed_(seed) {}

    [[nodiscard]] std::string_view name() const noexcept override {
        return "side_information";
    }

    [[nodiscard]] AlgorithmResult run(std::size_t n, Oracle& oracle,
                                      Proxy* proxy = nullptr) const override {
        if (n == 0 || proxy == nullptr)
            throw std::invalid_argument("Side Information requires elements and a proxy");

        std::vector<ElementId> order(n);
        std::iota(order.begin(), order.end(), ElementId{0});
        std::mt19937_64 rng(seed_);
        std::shuffle(order.begin(), order.end(), rng);
        std::vector<ClusterId> labels(n, n);
        std::vector<Cluster> clusters;
        Histogram within{}, between{};
        std::size_t queries = 0;
        const double log_n = std::log(static_cast<double>(n));
        constexpr double growth_constant = 4.0;
        const double initial_size = growth_constant * log_n;

        const auto score_bin = [proxy](ElementId a, ElementId b) {
            const auto score = proxy->score(std::min(a, b), std::max(a, b));
            if (!std::isfinite(score))
                throw std::invalid_argument("Side Information requires finite proxy scores");
            const double unit = 0.5 + std::atan(score) / std::numbers::pi;
            return std::min(bin_count - 1, static_cast<std::size_t>(unit * bin_count));
        };

        const auto classify = [&](ElementId point, ClusterId first) {
            auto assigned = clusters.size();
            for (std::size_t offset = 0; offset < clusters.size(); ++offset) {
                const auto id = (first + offset) % clusters.size();
                if (clusters[id].retired) continue;
                ++queries;
                if (oracle.query(point, clusters[id].members.front())) {
                    assigned = id;
                    break;
                }
            }
            if (assigned == clusters.size()) clusters.emplace_back();
            for (ClusterId id = 0; id < clusters.size(); ++id) {
                if (clusters[id].retired) continue;
                for (const auto other : clusters[id].members) {
                    const auto bin = score_bin(point, other);
                    if (id == assigned) {
                        ++within[bin];
                        ++clusters[id].histogram[bin];
                    } else {
                        ++between[bin];
                        ++clusters[id].cross[bin];
                        ++clusters[assigned].cross[bin];
                    }
                }
            }
            labels[point] = assigned;
            clusters[assigned].members.push_back(point);
        };

        std::size_t next = 0;
        while (true) {
            while (next < n && labels[order[next]] != n) ++next;
            if (next == n) break;
            const double separation = hellinger_squared(within, between);
            const double target = separation > 0 ? initial_size / separation
                                                  : static_cast<double>(n) + 1;
            const auto grown = std::find_if(clusters.begin(), clusters.end(),
                [target](const Cluster& cluster) {
                    return !cluster.retired && cluster.members.size() >= target;
                });
            if (grown == clusters.end()) {
                classify(order[next++], 0);
                continue;
            }

            const auto id = static_cast<ClusterId>(grown - clusters.begin());
            const double center = 4 * std::sqrt(separation) / growth_constant;
            const double margin = 2 * separation / (growth_constant * std::sqrt(log_n));
            std::vector<ElementId> waiting;
            // Keep the confirmed seed membership fixed throughout this scan.
            for (const auto point : order) {
                if (labels[point] != n) continue;
                Histogram incident{};
                for (const auto member : grown->members)
                    ++incident[score_bin(point, member)];
                const auto distance = hellinger_squared(incident, grown->histogram);
                if (distance <= center - margin) labels[point] = id;
                else if (distance <= center + margin) waiting.push_back(point);
            }
            for (const auto point : waiting) classify(point, id);

            // Keep only histograms of active, oracle-confirmed points.
            auto& finished = clusters[id];
            for (std::size_t bin = 0; bin < bin_count; ++bin) {
                within[bin] -= finished.histogram[bin];
                between[bin] -= finished.cross[bin];
            }
            for (ClusterId other = 0; other < clusters.size(); ++other) {
                if (other == id || clusters[other].retired) continue;
                for (const auto a : finished.members)
                    for (const auto b : clusters[other].members)
                        --clusters[other].cross[score_bin(a, b)];
            }
            finished.retired = true;
        }

        return AlgorithmResult(Partition(std::move(labels)),
                               std::vector<std::size_t>(queries, 1));
    }

private:
    static constexpr std::size_t bin_count = 16;
    using Histogram = std::array<std::size_t, bin_count>;

    struct Cluster {
        std::vector<ElementId> members;
        Histogram histogram{};
        Histogram cross{};
        bool retired = false;
    };

    static double hellinger_squared(const Histogram& a, const Histogram& b) {
        const auto total_a = std::accumulate(a.begin(), a.end(), std::size_t{0});
        const auto total_b = std::accumulate(b.begin(), b.end(), std::size_t{0});
        if (total_a == 0 || total_b == 0) return 0;
        double distance = 0;
        for (std::size_t bin = 0; bin < bin_count; ++bin) {
            const double difference = std::sqrt(static_cast<double>(a[bin]) / total_a)
                                    - std::sqrt(static_cast<double>(b[bin]) / total_b);
            distance += difference * difference;
        }
        return std::clamp(distance / 2, 0.0, 1.0);
    }

    std::uint64_t seed_;
};

} // namespace pl
