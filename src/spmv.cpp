#include "sparsebench/spmv.hpp"

#include <numeric>
#include <stdexcept>

#ifdef SPARSEBENCH_HAS_OPENMP
#include <omp.h>
#endif

namespace sparsebench {

void spmv_serial(const CSRMatrix& matrix,
                 const std::vector<double>& x,
                 std::vector<double>& y) {
    validate_csr(matrix);
    if (x.size() != static_cast<std::size_t>(matrix.ncols)) {
        throw std::runtime_error("SpMV input vector size does not match matrix ncols");
    }

    y.assign(static_cast<std::size_t>(matrix.nrows), 0.0);
    for (std::int64_t row = 0; row < matrix.nrows; ++row) {
        double sum = 0.0;
        const auto begin = matrix.row_ptr[static_cast<std::size_t>(row)];
        const auto end = matrix.row_ptr[static_cast<std::size_t>(row + 1)];
        for (auto k = begin; k < end; ++k) {
            const auto idx = static_cast<std::size_t>(k);
            sum += matrix.values[idx] *
                   x[static_cast<std::size_t>(matrix.col_idx[idx])];
        }
        y[static_cast<std::size_t>(row)] = sum;
    }
}

void spmv_omp(const CSRMatrix& matrix,
              const std::vector<double>& x,
              std::vector<double>& y,
              int threads) {
    if (threads <= 0) {
        throw std::runtime_error("OpenMP thread count must be positive");
    }
    validate_csr(matrix);
    if (x.size() != static_cast<std::size_t>(matrix.ncols)) {
        throw std::runtime_error("SpMV input vector size does not match matrix ncols");
    }

    y.assign(static_cast<std::size_t>(matrix.nrows), 0.0);

#ifdef SPARSEBENCH_HAS_OPENMP
#pragma omp parallel for schedule(static) num_threads(threads)
    for (std::int64_t row = 0; row < matrix.nrows; ++row) {
        double sum = 0.0;
        const auto begin = matrix.row_ptr[static_cast<std::size_t>(row)];
        const auto end = matrix.row_ptr[static_cast<std::size_t>(row + 1)];
        for (auto k = begin; k < end; ++k) {
            const auto idx = static_cast<std::size_t>(k);
            sum += matrix.values[idx] *
                   x[static_cast<std::size_t>(matrix.col_idx[idx])];
        }
        y[static_cast<std::size_t>(row)] = sum;
    }
#else
    (void)threads;
    spmv_serial(matrix, x, y);
#endif
}

double checksum(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0);
}

}  // namespace sparsebench
