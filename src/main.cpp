#include "algorithms/black_limited_round.hpp"
#include "algorithms/edge_ordering.hpp"
#include "benchmarks/milestone_runner.hpp"
#include "benchmarks/proxy_quality.hpp"

#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::uint64_t parse_nonnegative(const std::string& s) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) throw std::invalid_argument("Expected nonnegative integer");
    std::size_t pos = 0;
    const auto value = std::stoull(s, &pos, 10);
    if (pos != s.size()) throw std::invalid_argument("Invalid integer: " + s);
    return value;
}
double parse_real(const std::string& value) {
    std::size_t pos = 0;
    const double number = std::stod(value, &pos);
    if (pos != value.size() || !std::isfinite(number))
        throw std::invalid_argument("Invalid number: " + value);
    return number;
}

pl::ScoreDistribution parse_distribution(const std::string& value) {
    using Family = pl::ScoreDistribution::Family;
    const auto first = value.find(',');
    const auto second = value.find(',', first == std::string::npos ? 0 : first + 1);
    if (first == std::string::npos || second == std::string::npos)
        throw std::invalid_argument("Expected family,a,b");
    const auto family = value.substr(0, first);
    Family type;
    if (family == "gaussian") type = Family::gaussian;
    else if (family == "uniform") type = Family::uniform;
    else if (family == "beta") type = Family::beta;
    else throw std::invalid_argument("Unknown score family: " + family);
    return {type, parse_real(value.substr(first + 1, second - first - 1)),
            parse_real(value.substr(second + 1))};
}
}

int main(int argc, char** argv) {
    try {
        pl::BaselineExperiment cfg;
        std::string algorithm = "representative_baseline";
        std::optional<std::size_t> rounds;
        std::optional<std::size_t> cluster_bound;
        std::size_t quality_samples = 0;
        std::string output = "results/raw/milestone1.csv";
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "Usage: partition_experiment [--n N] [--k K] [--seed S]"
                             " [--output FILE]\n"
                             "  [--algorithm representative_baseline|black_limited_round|edge_ordering]\n"
                             "  [--rounds R] [--k-bound K] (Black only)\n"
                             "  [--shape balanced|uniform|zipf|power-law|one-large|singleton-heavy]\n"
                             "  [--exponent X] [--proxy-seed S] [--quality-samples N]\n"
                             "  [--within FAMILY,A,B] [--between FAMILY,A,B]\n"
                             "  Families: gaussian (mean,sigma), uniform (low,high), beta (alpha,beta)\n";
                return 0;
            }
            if (i + 1 >= argc) throw std::invalid_argument("Missing value for " + arg);
            const std::string value = argv[++i];
            if (arg == "--n") cfg.n = parse_nonnegative(value);
            else if (arg == "--k") cfg.k = parse_nonnegative(value);
            else if (arg == "--seed") cfg.seed = parse_nonnegative(value);
            else if (arg == "--algorithm") algorithm = value;
            else if (arg == "--rounds") rounds = parse_nonnegative(value);
            else if (arg == "--k-bound") cluster_bound = parse_nonnegative(value);
            else if (arg == "--shape") cfg.shape = value;
            else if (arg == "--exponent") cfg.exponent = parse_real(value);
            else if (arg == "--proxy-seed") cfg.proxy_seed = parse_nonnegative(value);
            else if (arg == "--within") cfg.within = parse_distribution(value);
            else if (arg == "--between") cfg.between = parse_distribution(value);
            else if (arg == "--quality-samples") quality_samples = parse_nonnegative(value);
            else if (arg == "--output") output = value;
            else throw std::invalid_argument("Unknown option: " + arg);
        }
        const auto result = [&] {
            if (algorithm == "black_limited_round") {
                const auto bound = cluster_bound.value_or(cfg.k);
                const auto budget = rounds.value_or(2);
                if (bound < cfg.k) throw std::invalid_argument("Require k-bound >= k");
                auto run = pl::run_experiment(cfg, pl::BlackLimitedRound(bound, budget));
                run.requested_rounds = budget;
                run.cluster_bound = bound;
                return run;
            }
            if (rounds || cluster_bound)
                throw std::invalid_argument("--rounds and --k-bound require black_limited_round");
            if (algorithm == "edge_ordering") return pl::run_experiment(cfg, pl::EdgeOrdering{});
            if (algorithm == "representative_baseline") return pl::run_baseline_experiment(cfg);
            throw std::invalid_argument("Unknown algorithm: " + algorithm);
        }();
        pl::write_baseline_csv(output, result);
        std::cout << "Algorithm: " << result.algorithm << '\n'
                  << "n=" << cfg.n << " k=" << cfg.k << " seed=" << cfg.seed << '\n'
                  << "Exact recovery: " << (result.metrics.exact_recovery ? "yes" : "no") << '\n'
                  << "Oracle queries: " << result.oracle_queries << '\n'
                  << "Adaptive rounds: " << result.adaptive_rounds << '\n'
                  << "Pairwise F1: " << result.metrics.pairwise_f1 << '\n'
                  << "ARI: " << result.metrics.adjusted_rand_index << '\n'
                  << "CSV: " << output << '\n';
        if (result.preprocessing_ms)
            std::cout << "Preprocessing (ms): " << *result.preprocessing_ms << '\n';
        if (result.requested_rounds != 0) {
            std::cout << "Queries per round:";
            for (const auto count : result.queries_per_round) std::cout << ' ' << count;
            std::cout << '\n';
        }
        if (quality_samples) {
            using Family = pl::ScoreDistribution::Family;
            pl::report_proxy_quality(
                cfg.within.value_or(pl::ScoreDistribution{Family::gaussian, 0.8, 0.1}),
                cfg.between.value_or(pl::ScoreDistribution{Family::gaussian, 0.2, 0.1}),
                quality_samples, cfg.proxy_seed.value_or(cfg.seed), std::cout);
        }
        return result.metrics.exact_recovery ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
