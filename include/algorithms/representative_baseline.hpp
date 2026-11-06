#pragma once

#include "core/algorithm.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace pl {

// Correct but deliberately simple reference: one representative per discovered
// cluster, compare each new element against representatives until one matches.
// Every query is considered its own adaptive round (conservative accounting).
class RepresentativeBaseline final : public Algorithm {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "representative_baseline";
    }

    [[nodiscard]] AlgorithmResult run(std::size_t n, Oracle& oracle,
                                      Proxy* proxy = nullptr) const override {
        (void)proxy; // Intentionally oracle-only; useful as a reference baseline.
        if (n == 0) throw std::invalid_argument("n must be positive");
        std::vector<ClusterId> labels(n);
        std::vector<ElementId> representatives{0};
        std::vector<std::size_t> rounds;
        labels[0] = 0;
        for (ElementId i = 1; i < n; ++i) {
            bool matched = false;
            for (std::size_t c = 0; c < representatives.size(); ++c) {
                const bool same = oracle.query(i, representatives[c]);
                rounds.push_back(1);
                if (same) {
                    labels[i] = c;
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                labels[i] = representatives.size();
                representatives.push_back(i);
            }
        }
        return AlgorithmResult(Partition(std::move(labels)), std::move(rounds));
    }
};

} // namespace pl
