#pragma once

#include "algorithms/representative_baseline.hpp"
#include "core/exact_oracle.hpp"
#include "core/metrics.hpp"
#include "core/synthetic_proxy.hpp"
#include "benchmarks/synthetic_partition.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace pl {

struct BaselineExperiment {
    std::size_t n = 12;
    std::size_t k = 3;
    std::uint64_t seed = 42;
    std::string shape = "balanced";
    double exponent = 2.0;
    std::optional<ScoreDistribution> within = std::nullopt;
    std::optional<ScoreDistribution> between = std::nullopt;
    std::optional<std::uint64_t> proxy_seed = std::nullopt;
};

inline Partition make_balanced_partition(const BaselineExperiment& cfg) {
    return make_synthetic_partition(cfg.n, cfg.k, cfg.seed, "balanced");
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
    std::string algorithm;
    std::vector<std::size_t> queries_per_round;
    std::size_t requested_rounds = 0;
    std::size_t cluster_bound = 0;
    std::optional<double> preprocessing_ms = std::nullopt;
};

inline BaselineExperimentResult run_experiment(const BaselineExperiment& cfg,
                                               const Algorithm& algorithm) {
    const Partition truth = make_synthetic_partition(cfg.n, cfg.k, cfg.seed, cfg.shape, cfg.exponent);
    ExactOracle oracle(truth);
    SyntheticProxy proxy(truth, {.seed = cfg.proxy_seed.value_or(cfg.seed), .cache = false,
                                 .within = cfg.within, .between = cfg.between});
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
            std::chrono::duration<double, std::milli>(finished - started).count(),
            std::string(algorithm.name()), answer.queries_per_round, 0, 0,
            answer.preprocessing_ms};
}

inline BaselineExperimentResult run_baseline_experiment(const BaselineExperiment& cfg) {
    return run_experiment(cfg, RepresentativeBaseline{});
}

inline void write_baseline_csv(const std::filesystem::path& path,
                                const BaselineExperimentResult& r) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open output CSV: " + path.string());
    out << "algorithm,dataset,n,k,seed,oracle_queries,unique_oracle_queries,"
           "proxy_accesses,proxy_computations,adaptive_rounds,runtime_ms,"
           "exact_recovery,pairwise_precision,pairwise_recall,pairwise_f1,adjusted_rand_index";
    if (r.preprocessing_ms) out << ",preprocessing_ms";
    if (r.requested_rounds != 0) out << ",requested_rounds,cluster_bound,queries_per_round";
    out << '\n';
    out << r.algorithm << ',' << r.config.shape << "_synthetic," << r.config.n << ','
        << r.config.k << ',' << r.config.seed << ',' << r.oracle_queries << ','
        << r.unique_oracle_queries << ',' << r.proxy_accesses << ','
        << r.proxy_computations << ',' << r.adaptive_rounds << ','
        << std::setprecision(17) << r.runtime_ms << ','
        << (r.metrics.exact_recovery ? 1 : 0) << ','
        << r.metrics.pairwise_precision << ',' << r.metrics.pairwise_recall << ','
        << r.metrics.pairwise_f1 << ',' << r.metrics.adjusted_rand_index;
    if (r.preprocessing_ms) out << ',' << *r.preprocessing_ms;
    if (r.requested_rounds != 0) {
        out << ',' << r.requested_rounds << ',' << r.cluster_bound << ',';
        const char* separator = "";
        for (const auto count : r.queries_per_round) {
            out << separator << count;
            separator = "|";
        }
    }
    out << '\n';
    if (!out) throw std::runtime_error("Failed to write CSV: " + path.string());
}

} // namespace pl
