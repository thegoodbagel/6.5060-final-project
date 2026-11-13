#include "algorithms/node_ordering.hpp"
#include "benchmarks/milestone_runner.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Node Ordering ranks nodes and queries each candidate cluster's best member") {
    class FixedProxy final : public pl::Proxy {
    public:
        double score(pl::ElementId a, pl::ElementId b) override {
            return weights[b][a];
        }
        std::uint64_t access_count() const override { return 0; }
        std::uint64_t computation_count() const override { return 0; }
        void reset_counters() override {}
        double weights[4][4] = {{}, {9}, {7, 1}, {6, 8, 0}};
    } proxy;
    class RecordingOracle final : public pl::Oracle {
    public:
        bool query(pl::ElementId a, pl::ElementId b) override {
            calls.emplace_back(a, b);
            return a != 2 && b != 2;
        }
        std::uint64_t query_count() const override { return calls.size(); }
        std::uint64_t unique_query_count() const override { return calls.size(); }
        void reset_counters() override { calls.clear(); }
        std::vector<std::pair<pl::ElementId, pl::ElementId>> calls;
    } oracle;
    const auto result = pl::NodeOrdering{}.run(4, oracle, &proxy);
    // Totals 22, 18, 8, 14 give order 0, 1, 3, 2.
    REQUIRE(oracle.calls == (decltype(oracle.calls){{1, 0}, {3, 1}, {2, 0}}));
    REQUIRE(result.partition.labels() == std::vector<pl::ClusterId>{0, 0, 1, 0});
}

TEST_CASE("Node Ordering recovers partitions even with uninformative scores") {
    for (const auto k : {1U, 5U, 20U}) {
        for (std::uint64_t seed = 0; seed < 12; ++seed) {
            const auto result = pl::run_experiment(
                {.n = 20, .k = k, .seed = seed, .shape = "zipf",
                 .within = pl::ScoreDistribution{pl::ScoreDistribution::Family::uniform, 0, 1},
                 .between = pl::ScoreDistribution{pl::ScoreDistribution::Family::uniform, 0, 1}},
                pl::NodeOrdering{});
            REQUIRE(result.metrics.exact_recovery);
            REQUIRE(result.oracle_queries == result.unique_oracle_queries);
            REQUIRE(result.oracle_queries == result.adaptive_rounds);
            REQUIRE(result.proxy_accesses == 190);
            REQUIRE(result.oracle_queries <= 20 * k);
        }
    }
}
