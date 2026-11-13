#include "algorithms/edge_ordering.hpp"
#include "benchmarks/milestone_runner.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Edge Ordering infers positive and negative pairs after merging") {
    const pl::Partition truth({0, 1, 0, 0});
    pl::SyntheticProxy proxy(truth, {.within_mean = 0, .between_mean = 1,
                                    .standard_deviation = 0});
    pl::ExactOracle oracle(truth);
    const auto result = pl::EdgeOrdering{}.run(4, oracle, &proxy);
    // Negative edges rank first and cannot be inferred before the positive merges.
    REQUIRE(oracle.query_count() == 5);
    REQUIRE(oracle.unique_query_count() == 5);
    REQUIRE(proxy.access_count() == 6);
    REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);

    pl::SyntheticProxy good(truth, {.standard_deviation = 0});
    oracle.reset_counters();
    const auto high_first = pl::EdgeOrdering{}.run(4, oracle, &good);
    REQUIRE(oracle.query_count() == 3);
    REQUIRE(high_first.adaptive_rounds() == 3);
}

TEST_CASE("Edge Ordering transfers negative constraints across positive merges") {
    class OrderedProxy final : public pl::Proxy {
    public:
        double score(pl::ElementId a, pl::ElementId b) override {
            ++accesses;
            if (a == 1 && b == 2) return 3;
            if (a == 0 && b == 1) return 2;
            return 1;
        }
        std::uint64_t access_count() const override { return accesses; }
        std::uint64_t computation_count() const override { return accesses; }
        void reset_counters() override { accesses = 0; }
        std::uint64_t accesses = 0;
    } proxy;
    const pl::Partition truth({0, 0, 1});
    pl::ExactOracle oracle(truth);
    const auto result = pl::EdgeOrdering{}.run(3, oracle, &proxy);
    REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);
    REQUIRE(oracle.query_count() == 2);
}

TEST_CASE("Edge Ordering recovers synthetic shapes with a noisy proxy") {
    for (const auto shape : {"balanced", "zipf", "singleton-heavy", "one-large"}) {
        for (const auto k : {1U, 6U, 30U}) {
            for (std::uint64_t seed = 0; seed < 10; ++seed) {
                const auto result = pl::run_experiment(
                    {.n = 30, .k = k, .seed = seed, .shape = shape},
                    pl::EdgeOrdering{});
                REQUIRE(result.metrics.exact_recovery);
                REQUIRE(result.oracle_queries == result.unique_oracle_queries);
                REQUIRE(result.oracle_queries == result.adaptive_rounds);
                REQUIRE(result.proxy_accesses == 435);
                REQUIRE(result.proxy_computations == 435);
            }
        }
    }
    pl::ExactOracle oracle(pl::Partition({0}));
    REQUIRE_THROWS_AS(pl::EdgeOrdering{}.run(1, oracle), std::invalid_argument);
}
