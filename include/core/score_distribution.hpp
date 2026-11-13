#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace pl {

struct ScoreDistribution {
    enum class Family { gaussian, uniform, beta };
    Family family;
    double a;
    double b;

    void validate() const {
        if (!std::isfinite(a) || !std::isfinite(b))
            throw std::invalid_argument("Score parameters must be finite");
        if ((family == Family::gaussian && b <= 0) ||
            (family == Family::uniform && (!(a < b) || !std::isfinite(b - a))) ||
            (family == Family::beta && (a <= 0 || b <= 0 || !std::isfinite(a + b))))
            throw std::invalid_argument("Require Gaussian sigma > 0, uniform low < high, or positive Beta shapes");
    }

    double sample(std::mt19937_64& rng) const {
        switch (family) {
        case Family::gaussian:
            return std::normal_distribution<double>(a, b)(rng);
        case Family::uniform:
            return std::uniform_real_distribution<double>(a, b)(rng);
        case Family::beta: {
            // Raising a Gamma(shape + 1) variate in log space avoids underflow
            // when either shape is below one.
            const auto log_gamma = [&rng](double shape) {
                if (shape >= 1) return std::log(static_cast<long double>(
                    std::gamma_distribution<double>(shape)(rng)));
                const auto u = std::uniform_real_distribution<double>(0, 1)(rng);
                return std::log(static_cast<long double>(
                    std::gamma_distribution<double>(shape + 1)(rng)))
                    + std::log1p(-static_cast<long double>(u)) / shape;
            };
            const auto x = log_gamma(a);
            const auto y = log_gamma(b);
            return static_cast<double>(1 / (1 + std::exp(y - x)));
        }
        }
        throw std::invalid_argument("Unknown score family");
    }

    double log_density(double x) const {
        const auto outside = -std::numeric_limits<double>::infinity();
        switch (family) {
        case Family::gaussian: {
            const double z = (x - a) / b;
            return -0.5 * z * z - std::log(b) - 0.9189385332046727;
        }
        case Family::uniform:
            return x >= a && x <= b ? -std::log(b - a) : outside;
        case Family::beta:
            if (x < 0 || x > 1) return outside;
            x = std::max(std::numeric_limits<double>::denorm_min(),
                         std::min(x, std::nextafter(1.0, 0.0)));
            return (a - 1) * std::log(x) + (b - 1) * std::log1p(-x)
                   + std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b);
        }
        return outside;
    }
};

} // namespace pl
