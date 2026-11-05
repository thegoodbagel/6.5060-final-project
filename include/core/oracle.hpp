#pragma once

#include "core/partition.hpp"

#include <cstdint>

namespace pl {

// Algorithms receive this interface; they must not receive the ground truth.
class Oracle {
public:
    virtual ~Oracle() = default;

    virtual bool query(ElementId a, ElementId b) = 0;
    [[nodiscard]] virtual std::uint64_t query_count() const = 0;
    [[nodiscard]] virtual std::uint64_t unique_query_count() const = 0;
    virtual void reset_counters() = 0;
};

} // namespace pl

