#include "benchmarks/milestone_runner.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::uint64_t parse_nonnegative(const std::string& s) {
    if (s.empty() || s[0] == '-') throw std::invalid_argument("Expected nonnegative integer");
    std::size_t pos = 0;
    const auto value = std::stoull(s, &pos, 10);
    if (pos != s.size()) throw std::invalid_argument("Invalid integer: " + s);
    return value;
}
}

int main(int argc, char** argv) {
    try {
        pl::BaselineExperiment cfg;
        std::string output = "results/raw/milestone1.csv";
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "Usage: partition_experiment [--n N] [--k K] [--seed S]"
                             " [--output FILE]\n";
                return 0;
            }
            if (i + 1 >= argc) throw std::invalid_argument("Missing value for " + arg);
            const std::string value = argv[++i];
            if (arg == "--n") cfg.n = parse_nonnegative(value);
            else if (arg == "--k") cfg.k = parse_nonnegative(value);
            else if (arg == "--seed") cfg.seed = parse_nonnegative(value);
            else if (arg == "--output") output = value;
            else throw std::invalid_argument("Unknown option: " + arg);
        }
        const auto result = pl::run_baseline_experiment(cfg);
        pl::write_baseline_csv(output, result);
        std::cout << "Algorithm: representative_baseline\n"
                  << "n=" << cfg.n << " k=" << cfg.k << " seed=" << cfg.seed << '\n'
                  << "Exact recovery: " << (result.metrics.exact_recovery ? "yes" : "no") << '\n'
                  << "Oracle queries: " << result.oracle_queries << '\n'
                  << "Adaptive rounds: " << result.adaptive_rounds << '\n'
                  << "Pairwise F1: " << result.metrics.pairwise_f1 << '\n'
                  << "ARI: " << result.metrics.adjusted_rand_index << '\n'
                  << "CSV: " << output << '\n';
        return result.metrics.exact_recovery ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
