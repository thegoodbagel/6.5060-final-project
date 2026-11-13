#pragma once

#include "core/algorithm.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <unordered_set>

namespace pl {

// Edge Ordering as described by Mazumdar and Saha (2017), Section 2.2,
// using the positive and negative transitive inference of Wang et al. (2013).
class EdgeOrdering final : public Algorithm {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "edge_ordering";
    }

    [[nodiscard]] AlgorithmResult run(std::size_t n, Oracle& oracle,
                                      Proxy* proxy = nullptr) const override {
        if (n == 0 || proxy == nullptr)
            throw std::invalid_argument("Edge Ordering requires elements and a proxy");

        struct Edge {
            ElementId a;
            ElementId b;
            double score;
        };
        const auto started = std::chrono::steady_clock::now();
        std::vector<Edge> edges;
        // Divide first, then check the product against the vector capacity.
        const auto factor = n % 2 == 0 ? n / 2 : n;
        const auto other = n % 2 == 0 ? n - 1 : (n - 1) / 2;
        if (other > edges.max_size() / factor)
            throw std::length_error("Too many candidate edges");
        edges.reserve(factor * other);
        for (ElementId a = 0; a < n; ++a) {
            for (ElementId b = a + 1; b < n; ++b) {
                const auto score = proxy->score(a, b);
                if (!std::isfinite(score))
                    throw std::invalid_argument("Edge Ordering requires finite proxy scores");
                edges.push_back({a, b, score});
            }
        }
        std::stable_sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
            return a.score > b.score;
        });
        const auto sorted = std::chrono::steady_clock::now();

        std::vector<ClusterId> parent(n);
        std::iota(parent.begin(), parent.end(), ClusterId{0});
        std::vector<std::size_t> size(n, 1);
        std::vector<std::unordered_set<ClusterId>> different(n);
        const auto root = [&parent](ClusterId id) {
            while (id != parent[id]) {
                parent[id] = parent[parent[id]];
                id = parent[id];
            }
            return id;
        };

        std::size_t queries = 0;
        for (const auto& edge : edges) {
            auto a = root(edge.a);
            auto b = root(edge.b);
            if (a == b || different[a].contains(b)) continue;
            ++queries;
            if (!oracle.query(edge.a, edge.b)) {
                different[a].insert(b);
                different[b].insert(a);
                continue;
            }

            if (size[a] < size[b]) std::swap(a, b);
            parent[b] = a;
            size[a] += size[b];
            for (const auto neighbor : different[b]) {
                different[neighbor].erase(b);
                different[neighbor].insert(a);
                different[a].insert(neighbor);
            }
            different[b].clear();
        }
        for (ClusterId id = 0; id < n; ++id) parent[id] = root(id);
        AlgorithmResult result(Partition(std::move(parent)),
                               std::vector<std::size_t>(queries, 1));
        result.preprocessing_ms =
            std::chrono::duration<double, std::milli>(sorted - started).count();
        return result;
    }
};

} // namespace pl
