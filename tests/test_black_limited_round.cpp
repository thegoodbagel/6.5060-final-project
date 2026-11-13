#include "algorithms/black_limited_round.hpp"
#include "benchmarks/milestone_runner.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Black limited-round follows its base cases") {
    const pl::Partition truth({8, 9, 8, 10, 9});
    for (const auto rounds : {1U, 3U}) {
        pl::ExactOracle oracle(truth);
        const auto result = pl::BlackLimitedRound(3, rounds).run(truth.size(), oracle);
        REQUIRE(result.queries_per_round == std::vector<std::size_t>{10});
        REQUIRE(oracle.unique_query_count() == 10);
        REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);
    }
    pl::ExactOracle single(pl::Partition({0}));
    REQUIRE(pl::BlackLimitedRound(1, 3).run(1, single).adaptive_rounds() == 0);
    REQUIRE_THROWS_AS(pl::BlackLimitedRound(0, 1), std::invalid_argument);
    REQUIRE_THROWS_AS(pl::BlackLimitedRound(1, 0), std::invalid_argument);
}

TEST_CASE("Black limited-round uses the paper's block sizes and re-queries pairs") {
    std::vector<pl::ClusterId> labels(500);
    for (std::size_t i = 0; i < labels.size(); ++i) labels[i] = i % 10;
    const pl::Partition truth(std::move(labels));
    pl::ExactOracle oracle(truth);
    const auto result = pl::BlackLimitedRound(10, 3).run(truth.size(), oracle);
    // Nine blocks of 53 plus one of 23, then all pairs among 100 reps.
    REQUIRE(result.queries_per_round == (std::vector<std::size_t>{12655, 4950}));
    REQUIRE(oracle.query_count() == 17605);
    REQUIRE(oracle.unique_query_count() == 17155);
    REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);
}

TEST_CASE("Black limited-round recovers randomized partitions within budget") {
    for (const auto shape : {"balanced", "zipf", "singleton-heavy", "one-large"}) {
        for (std::uint64_t seed = 0; seed < 8; ++seed) {
            const auto truth = pl::make_synthetic_partition(240, 4, seed, shape);
            for (const auto rounds : {1U, 2U, 3U, 128U}) {
                pl::ExactOracle oracle(truth);
                const auto result = pl::BlackLimitedRound(5, rounds).run(truth.size(), oracle);
                REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);
                REQUIRE(result.adaptive_rounds() <= rounds);
                REQUIRE(std::accumulate(result.queries_per_round.begin(),
                                        result.queries_per_round.end(), std::size_t{0})
                        == oracle.query_count());
            }
        }
    }
}

TEST_CASE("Black limited-round can use three batches") {
    const auto truth = pl::make_synthetic_partition(4096, 2, 42, "balanced");
    pl::ExactOracle oracle(truth);
    const auto result = pl::BlackLimitedRound(2, 3).run(truth.size(), oracle);
    REQUIRE(result.adaptive_rounds() == 3);
    REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);
}
