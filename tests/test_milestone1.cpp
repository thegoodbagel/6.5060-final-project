#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "algorithms/representative_baseline.hpp"
#include "benchmarks/milestone_runner.hpp"
#include "core/exact_oracle.hpp"
#include "core/metrics.hpp"

#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

TEST_CASE("Representative baseline recovers arbitrary cluster labels", "[baseline]") {
    const pl::Partition truth({7, 9, 7, 13, 9, 13});
    pl::ExactOracle oracle(truth);
    pl::RepresentativeBaseline algo;
    const auto result = algo.run(truth.size(), oracle);
    REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);
    REQUIRE(oracle.query_count() == 9);
    REQUIRE(oracle.unique_query_count() == 9);
    REQUIRE(result.adaptive_rounds() == 9);
}

TEST_CASE("Baseline handles one cluster, singletons, and n=1", "[baseline]") {
    for (const auto& truth : {pl::Partition({4, 4, 4, 4}),
                              pl::Partition({1, 2, 3, 4}), pl::Partition({99})}) {
        pl::ExactOracle oracle(truth);
        const auto result = pl::RepresentativeBaseline{}.run(truth.size(), oracle);
        REQUIRE(pl::evaluate_partition(truth, result.partition).exact_recovery);
        REQUIRE(result.queries_per_round.size() == oracle.query_count());
    }
}

TEST_CASE("Metrics invariant to relabeling", "[metrics]") {
    const pl::Partition truth({0, 0, 1, 1, 2});
    const auto m = pl::evaluate_partition(truth, pl::Partition({9, 9, 7, 7, 42}));
    REQUIRE(m.exact_recovery);
    REQUIRE(m.pairwise_precision == 1.0);
    REQUIRE(m.pairwise_recall == 1.0);
    REQUIRE(m.pairwise_f1 == 1.0);
    REQUIRE(m.adjusted_rand_index == 1.0);
}

TEST_CASE("Metrics detect merged clusters and singleton degeneracies", "[metrics]") {
    const auto m = pl::evaluate_partition(pl::Partition({0, 0, 1, 1}),
                                          pl::Partition({7, 7, 7, 7}));
    REQUIRE_FALSE(m.exact_recovery);
    REQUIRE(m.pairwise_precision == Catch::Approx(1.0 / 3.0));
    REQUIRE(m.pairwise_recall == 1.0);
    REQUIRE(m.pairwise_f1 == Catch::Approx(0.5));
    REQUIRE(m.adjusted_rand_index == Catch::Approx(0.0).margin(1e-12));
    const auto all_single = pl::evaluate_partition(pl::Partition({0, 1, 2}),
                                                   pl::Partition({7, 8, 9}));
    REQUIRE(all_single.exact_recovery);
    REQUIRE(all_single.pairwise_f1 == 1.0);
    REQUIRE(all_single.adjusted_rand_index == 1.0);
}

TEST_CASE("End-to-end experiment writes one CSV data row", "[experiment]") {
    const pl::BaselineExperiment cfg{12, 3, 1234};
    const auto r1 = pl::run_baseline_experiment(cfg);
    const auto r2 = pl::run_baseline_experiment(cfg);
    REQUIRE(r1.metrics.exact_recovery);
    REQUIRE(r1.metrics.adjusted_rand_index == 1.0);
    REQUIRE(r1.oracle_queries == r2.oracle_queries);
    REQUIRE(r1.proxy_accesses == 0); // Baseline ignores optional side information.
    const auto path = std::filesystem::temp_directory_path() / "pl_milestone1_test.csv";
    pl::write_baseline_csv(path, r1);
    std::ifstream input(path);
    REQUIRE(input.good());
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    REQUIRE(text.find("adjusted_rand_index") != std::string::npos);
    REQUIRE(std::count(text.begin(), text.end(), '\n') == 2);
    std::filesystem::remove(path);
}

TEST_CASE("Invalid configuration and inconsistent partition sizes fail", "[experiment][metrics]") {
    REQUIRE_THROWS_AS(pl::make_balanced_partition({3, 4, 1}), std::invalid_argument);
    REQUIRE_THROWS_AS(pl::make_balanced_partition({0, 0, 1}), std::invalid_argument);
    REQUIRE_THROWS_AS(pl::evaluate_partition(pl::Partition({0}), pl::Partition({0, 1})),
                      std::invalid_argument);
}
