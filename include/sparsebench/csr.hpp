#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sparsebench {

struct CSRMatrix {
    std::int64_t nrows = 0;
    std::int64_t ncols = 0;
    std::int64_t nnz = 0;
    std::vector<std::int64_t> row_ptr;
    std::vector<std::int32_t> col_idx;
    std::vector<double> values;
};

void validate_csr(const CSRMatrix& matrix);
std::string matrix_name_from_path(const std::string& path);

}  // namespace sparsebench
