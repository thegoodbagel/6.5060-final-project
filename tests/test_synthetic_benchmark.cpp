#include "benchmarks/proxy_quality.hpp"
#include "benchmarks/synthetic_partition.hpp"
#include "core/synthetic_proxy.hpp"

#include <catch2/catch_test_macros.hpp>

#include <sstream>

TEST_CASE("Synthetic shapes produce exactly k nonempty clusters") {
    for (const auto shape : {"balanced", "uniform", "zipf", "power-law",
                             "one-large", "singleton-heavy"}) {
        const auto first = pl::make_synthetic_partition(500, 20, 7, shape);
        REQUIRE(first.size() == 500);
        REQUIRE(first.num_clusters() == 20);
        REQUIRE(first.labels() == pl::make_synthetic_partition(500, 20, 7, shape).labels());
        REQUIRE(pl::make_synthetic_partition(4, 4, 7, shape).num_clusters() == 4);
        REQUIRE(pl::make_synthetic_partition(4, 1, 7, shape).num_clusters() == 1);
    }
}

TEST_CASE("Configured scores are symmetric and independent of access order") {
    using Family = pl::ScoreDistribution::Family;
    const pl::Partition truth({0, 0, 1, 1});
    const pl::SyntheticProxy::Parameters params{
        .seed = 1, .cache = true,
        .within = pl::ScoreDistribution{Family::beta, 8, 2},
        .between = pl::ScoreDistribution{Family::uniform, 0, 0.25}};
    pl::SyntheticProxy first(truth, params), second(truth, params);
    const auto positive = first.score(0, 1);
    const auto negative = second.score(0, 2);
    REQUIRE(positive >= 0);
    REQUIRE(positive <= 1);
    REQUIRE(negative >= 0);
    REQUIRE(negative <= 0.25);
    REQUIRE(first.score(2, 0) == negative);
    REQUIRE(second.score(1, 0) == positive);
    REQUIRE(first.score(1, 0) == positive);
    REQUIRE(first.access_count() == 3);
    REQUIRE(first.computation_count() == 2);
}

TEST_CASE("Proxy quality matches identical and disjoint supports") {
    using Family = pl::ScoreDistribution::Family;
    const pl::ScoreDistribution low{Family::uniform, 0, 0.4};
    const pl::ScoreDistribution high{Family::uniform, 0.6, 1};
    std::ostringstream disjoint;
    pl::report_proxy_quality(high, low, 100, 7, disjoint);
    REQUIRE(disjoint.str().starts_with("Squared Hellinger estimate: 1\nOverlap estimate: 0\nAUC estimate: 1\n"));
    std::ostringstream identical;
    pl::report_proxy_quality(low, low, 100, 7, identical);
    REQUIRE(identical.str().starts_with("Squared Hellinger estimate: 0\nOverlap estimate: 1\n"));
}
