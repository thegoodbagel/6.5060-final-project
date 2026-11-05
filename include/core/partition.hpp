#pragma once

#include <cstddef>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pl {

using ElementId = std::size_t;
using ClusterId = std::size_t;

// Cluster IDs are labels, not array indices; gaps are allowed.
class Partition {
public:
    explicit Partition(std::vector<ClusterId> labels)
        : labels_(std::move(labels)) {
        if (labels_.empty()) {
            throw std::invalid_argument("Partition must contain at least one element");
        }
        num_clusters_ = std::unordered_set<ClusterId>(
            labels_.begin(), labels_.end()).size();
    }

    [[nodiscard]] std::size_t size() const noexcept { return labels_.size(); }
    [[nodiscard]] std::size_t num_clusters() const noexcept { return num_clusters_; }

    [[nodiscard]] ClusterId cluster_of(ElementId element) const {
        return labels_.at(element);
    }

    [[nodiscard]] bool same_cluster(ElementId a, ElementId b) const {
        return cluster_of(a) == cluster_of(b);
    }

    [[nodiscard]] const std::vector<ClusterId>& labels() const noexcept {
        return labels_;
    }

private:
    std::vector<ClusterId> labels_;
    std::size_t num_clusters_ = 0;
};

} // namespace pl

