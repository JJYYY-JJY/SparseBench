#pragma once

#include <string>

#include "sparsebench/csr.hpp"

namespace sparsebench {

CSRMatrix read_matrix_market_to_csr(const std::string& path);

}  // namespace sparsebench
