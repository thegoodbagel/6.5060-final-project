#pragma once

#include "core/oracle.hpp"
#include "core/proxy.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace pl {

struct Query {
    ElementId a;
    ElementId b;
};
using QueryBatch = std::vector<Query>;

// An algorithm returns labels for all n elements and the sizes of its adaptive
// oracle query rounds. Recovery correctness is assessed by the evaluator.
struct AlgorithmResult {
    Partition partition;
    std::vector<std::size_t> queries_per_round;

    explicit AlgorithmResult(Partition p,
                             std::vector<std::size_t> rounds = {})
        : partition(std::move(p)), queries_per_round(std::move(rounds)) {}

    [[nodiscard]] std::size_t adaptive_rounds() const noexcept {
        return queries_per_round.size();
    }
};

// Algorithm-specific settings belong in algorithm constructors, not the runner.
// The runner handles timing and query-counter snapshots externally.
class Algorithm {
public:
    virtual ~Algorithm() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual AlgorithmResult run(std::size_t n,
                                              Oracle& oracle,
                                              Proxy* proxy = nullptr) const = 0;
};

} // namespace pl

