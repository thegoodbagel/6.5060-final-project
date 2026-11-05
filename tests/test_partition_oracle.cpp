#include <catch2/catch_test_macros.hpp>

#include "core/exact_oracle.hpp"

#include <stdexcept>
#include <thread>
#include <vector>

TEST_CASE("Partition supports arbitrary cluster labels", "[partition]") {
    const pl::Partition p({7, 7, 42, 3, 42});
    REQUIRE(p.size() == 5);
    REQUIRE(p.num_clusters() == 3);
    REQUIRE(p.cluster_of(2) == 42);
    REQUIRE(p.same_cluster(0, 1));
    REQUIRE(p.same_cluster(2, 4));
    REQUIRE_FALSE(p.same_cluster(1, 2));
    REQUIRE_THROWS_AS(p.cluster_of(5), std::out_of_range);
    REQUIRE_THROWS_AS(p.same_cluster(0, 5), std::out_of_range);
}

TEST_CASE("Partition rejects no elements", "[partition]") {
    REQUIRE_THROWS_AS(pl::Partition(std::vector<pl::ClusterId>{}),
                      std::invalid_argument);
}

TEST_CASE("ExactOracle answers and counts repeated and reversed queries", "[oracle]") {
    pl::ExactOracle oracle(pl::Partition({8, 8, 4}));
    pl::Oracle& api = oracle;

    REQUIRE(api.query(0, 1));
    REQUIRE(api.query(1, 0));
    REQUIRE_FALSE(api.query(0, 2));
    REQUIRE(api.query(2, 2));
    REQUIRE(api.query_count() == 4);
    REQUIRE(api.unique_query_count() == 3);

    REQUIRE_THROWS_AS(api.query(0, 99), std::out_of_range);
    REQUIRE(api.query_count() == 4);
    REQUIRE(api.unique_query_count() == 3);

    api.reset_counters();
    REQUIRE(api.query_count() == 0);
    REQUIRE(api.unique_query_count() == 0);
    REQUIRE(api.query(0, 1));
    REQUIRE(api.unique_query_count() == 1);
}

TEST_CASE("ExactOracle counts concurrent queries safely", "[oracle][threading]") {
    pl::ExactOracle oracle(pl::Partition({0, 0, 1, 1}));
    constexpr int threads = 4;
    constexpr int per_thread = 1000;
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&oracle] {
            for (int i = 0; i < per_thread; ++i) {
                (void)oracle.query(0, 2);
                (void)oracle.query(1, 0);
            }
        });
    }
    for (auto& worker : workers) worker.join();
    REQUIRE(oracle.query_count() == 2 * threads * per_thread);
    REQUIRE(oracle.unique_query_count() == 2);
}

