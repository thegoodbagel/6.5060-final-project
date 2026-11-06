#pragma once

#include "core/partition.hpp"

#include <cstdint>

namespace pl {

// A larger score means stronger evidence that two elements belong together.
// Accesses and actual computations are counted separately for cached proxies.
class Proxy {
public:
    virtual ~Proxy() = default;
    virtual double score(ElementId a, ElementId b) = 0;
    [[nodiscard]] virtual std::uint64_t access_count() const = 0;
    [[nodiscard]] virtual std::uint64_t computation_count() const = 0;
    virtual void reset_counters() = 0;
};

} // namespace pl

