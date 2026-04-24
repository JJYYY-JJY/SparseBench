#pragma once

#include <vector>

#include "sparsebench/csr.hpp"

namespace sparsebench {

void spmv_serial(const CSRMatrix& matrix,
                 const std::vector<double>& x,
                 std::vector<double>& y);

void spmv_omp(const CSRMatrix& matrix,
              const std::vector<double>& x,
              std::vector<double>& y,
              int threads);

double checksum(const std::vector<double>& values);

}  // namespace sparsebench
