#include <catch2/catch_test_macros.hpp>

#include "core/algorithm.hpp"
#include "core/exact_oracle.hpp"
#include "core/synthetic_proxy.hpp"

#include <cmath>
#include <stdexcept>
#include <thread>
#include <vector>

TEST_CASE("SyntheticProxy is symmetric, deterministic, and independent of access order", "[proxy]") {
    const pl::Partition truth({0, 0, 1, 1});
    pl::SyntheticProxy a(truth, {.seed = 42});
    pl::SyntheticProxy b(truth, {.seed = 42});
    const auto x = a.score(0, 1);
    REQUIRE(a.score(1, 0) == x);
    (void)b.score(2, 3);
    REQUIRE(b.score(1, 0) == x);
    REQUIRE(a.score(0, 2) == b.score(2, 0));
    REQUIRE(a.access_count() == 3);
    REQUIRE(a.computation_count() == 3);
    REQUIRE_THROWS_AS(a.score(0, 10), std::out_of_range);
    REQUIRE(a.access_count() == 3);
}

TEST_CASE("SyntheticProxy separates within and between means when sigma is zero", "[proxy]") {
    pl::SyntheticProxy proxy(pl::Partition({1, 1, 2}),
       {.within_mean = 0.9, .between_mean = 0.1, .standard_deviation = 0.0});
    REQUIRE(proxy.score(0, 1) == 0.9);
    REQUIRE(proxy.score(0, 2) == 0.1);
    pl::SyntheticProxy::Parameters bad_params;
    bad_params.standard_deviation = -1.0;
    REQUIRE_THROWS_AS(pl::SyntheticProxy(pl::Partition({0}), bad_params),
                      std::invalid_argument);
}

TEST_CASE("SyntheticProxy caching and reset semantics", "[proxy]") {
    pl::SyntheticProxy proxy(pl::Partition({0, 0, 1}), {.seed = 7, .cache = true});
    const double first = proxy.score(0, 1);
    REQUIRE(proxy.score(1, 0) == first);
    REQUIRE(proxy.access_count() == 2);
    REQUIRE(proxy.computation_count() == 1);
    proxy.reset_counters();
    REQUIRE(proxy.access_count() == 0);
    REQUIRE(proxy.computation_count() == 0);
    REQUIRE(proxy.score(0, 1) == first);
    REQUIRE(proxy.computation_count() == 0);
    proxy.clear_cache();
    REQUIRE(proxy.score(0, 1) == first);
    REQUIRE(proxy.computation_count() == 1);
}

TEST_CASE("SyntheticProxy supports concurrent access", "[proxy][threading]") {
    pl::SyntheticProxy proxy(pl::Partition({1, 1, 2}), {.cache = true});
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 100; ++i) (void)proxy.score(0, 1);
        });
    }
    for (auto& thread : threads) thread.join();
    REQUIRE(proxy.access_count() == 400);
    REQUIRE(proxy.computation_count() == 1);
}

// Minimal fake implementation tests the API contract, not a published method.
class ProbeAlgorithm final : public pl::Algorithm {
public:
    std::string_view name() const noexcept override { return "probe"; }
    pl::AlgorithmResult run(std::size_t n, pl::Oracle& oracle,
                             pl::Proxy* proxy = nullptr) const override {
        if (n != 3) throw std::invalid_argument("probe expects 3 elements");
        if (proxy) (void)proxy->score(0, 1);
        const bool joined = oracle.query(0, 1);
        return pl::AlgorithmResult(pl::Partition(joined
            ? std::vector<pl::ClusterId>{0, 0, 1}
            : std::vector<pl::ClusterId>{0, 1, 2}), {1});
    }
};

TEST_CASE("Algorithm interface supports optional proxy and round accounting", "[algorithm]") {
    pl::ExactOracle oracle(pl::Partition({3, 3, 8}));
    pl::SyntheticProxy proxy(pl::Partition({3, 3, 8}));
    ProbeAlgorithm algo;
    const pl::Algorithm& api = algo;
    REQUIRE(api.name() == "probe");
    auto answer = api.run(3, oracle);
    REQUIRE(answer.partition.same_cluster(0, 1));
    REQUIRE(answer.adaptive_rounds() == 1);
    REQUIRE(answer.queries_per_round[0] == 1);
    REQUIRE(oracle.query_count() == 1);
    answer = api.run(3, oracle, &proxy);
    REQUIRE(proxy.access_count() == 1);
    REQUIRE(oracle.query_count() == 2);
}

