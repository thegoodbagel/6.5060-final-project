#include "algorithms/side_information.hpp"
#include "benchmarks/milestone_runner.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Side Information reduces oracle queries on clearly separated scores") {
    using Family = pl::ScoreDistribution::Family;
    const auto result = pl::run_experiment(
        {.n = 500, .k = 3, .seed = 42,
         .within = pl::ScoreDistribution{Family::uniform, 0.8, 0.81},
         .between = pl::ScoreDistribution{Family::uniform, 0.1, 0.11}},
        pl::SideInformation{});
    REQUIRE(result.metrics.exact_recovery);
    const auto baseline = pl::run_baseline_experiment({.n = 500, .k = 3, .seed = 42});
    REQUIRE(result.oracle_queries < baseline.oracle_queries);
    REQUIRE(result.oracle_queries == result.unique_oracle_queries);
    REQUIRE(result.oracle_queries == result.adaptive_rounds);
}

TEST_CASE("Side Information uses the oracle when histograms cannot separate") {
    for (const auto k : {1U, 5U, 20U}) {
        const pl::Partition truth = pl::make_synthetic_partition(20, k, 42, "balanced");
        pl::ExactOracle oracle(truth);
        pl::SyntheticProxy proxy(truth, {.within_mean = 0.5, .between_mean = 0.5,
                                        .standard_deviation = 0});
        const auto answer = pl::SideInformation{}.run(20, oracle, &proxy);
        REQUIRE(pl::evaluate_partition(truth, answer.partition).exact_recovery);
        REQUIRE(oracle.query_count() == answer.adaptive_rounds());
        REQUIRE(oracle.query_count() == oracle.unique_query_count());
    }
}
