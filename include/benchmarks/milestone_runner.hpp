#pragma once

#include "algorithms/representative_baseline.hpp"
#include "core/exact_oracle.hpp"
#include "core/metrics.hpp"
#include "core/synthetic_proxy.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace pl {

struct BaselineExperiment {
    std::size_t n = 12;
    std::size_t k = 3;
    std::uint64_t seed = 42;
};

inline Partition make_balanced_partition(const BaselineExperiment& cfg) {
    if (cfg.n == 0 || cfg.k == 0 || cfg.k > cfg.n)
        throw std::invalid_argument("Require 1 <= k <= n");
    std::vector<ClusterId> labels(cfg.n);
    for (std::size_t i = 0; i < cfg.n; ++i) labels[i] = i % cfg.k;
    std::mt19937_64 rng(cfg.seed);
    std::shuffle(labels.begin(), labels.end(), rng);
    return Partition(std::move(labels));
}

struct BaselineExperimentResult {
    BaselineExperiment config;
    PartitionMetrics metrics;
    std::uint64_t oracle_queries = 0;
    std::uint64_t unique_oracle_queries = 0;
    std::uint64_t proxy_accesses = 0;
    std::uint64_t proxy_computations = 0;
    std::size_t adaptive_rounds = 0;
    double runtime_ms = 0.0;
};

inline BaselineExperimentResult run_baseline_experiment(const BaselineExperiment& cfg) {
    const Partition truth = make_balanced_partition(cfg);
    ExactOracle oracle(truth);
    SyntheticProxy proxy(truth, {.seed = cfg.seed, .cache = true});
    RepresentativeBaseline algorithm;
    const auto started = std::chrono::steady_clock::now();
    const auto answer = algorithm.run(cfg.n, oracle, &proxy);
    const auto finished = std::chrono::steady_clock::now();
    if (answer.partition.size() != cfg.n)
        throw std::runtime_error("Algorithm returned the wrong number of labels");
    const auto counted = std::accumulate(answer.queries_per_round.begin(),
                                         answer.queries_per_round.end(), std::size_t{0});
    if (counted != oracle.query_count())
        throw std::runtime_error("Query rounds do not match the oracle counter");
    return {cfg, evaluate_partition(truth, answer.partition), oracle.query_count(),
            oracle.unique_query_count(), proxy.access_count(), proxy.computation_count(),
            answer.adaptive_rounds(),
            std::chrono::duration<double, std::milli>(finished - started).count()};
}

inline void write_baseline_csv(const std::filesystem::path& path,
                                const BaselineExperimentResult& r) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open output CSV: " + path.string());
    out << "algorithm,dataset,n,k,seed,oracle_queries,unique_oracle_queries,"
           "proxy_accesses,proxy_computations,adaptive_rounds,runtime_ms,"
           "exact_recovery,pairwise_precision,pairwise_recall,pairwise_f1,adjusted_rand_index\n";
    out << "representative_baseline,balanced_synthetic," << r.config.n << ','
        << r.config.k << ',' << r.config.seed << ',' << r.oracle_queries << ','
        << r.unique_oracle_queries << ',' << r.proxy_accesses << ','
        << r.proxy_computations << ',' << r.adaptive_rounds << ','
        << std::setprecision(17) << r.runtime_ms << ','
        << (r.metrics.exact_recovery ? 1 : 0) << ','
        << r.metrics.pairwise_precision << ',' << r.metrics.pairwise_recall << ','
        << r.metrics.pairwise_f1 << ',' << r.metrics.adjusted_rand_index << '\n';
    if (!out) throw std::runtime_error("Failed to write CSV: " + path.string());
}

} // namespace pl
