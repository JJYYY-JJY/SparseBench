#pragma once

#include <chrono>

namespace sparsebench {

class Timer {
public:
    Timer() : start_(Clock::now()) {}

    void reset() { start_ = Clock::now(); }

    double elapsed_ms() const {
        const auto end = Clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_;
};

}  // namespace sparsebench
