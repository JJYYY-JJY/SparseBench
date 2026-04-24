#include "sparsebench/csr.hpp"

#include <filesystem>
#include <stdexcept>

namespace sparsebench {

void validate_csr(const CSRMatrix& matrix) {
    if (matrix.nrows < 0 || matrix.ncols < 0 || matrix.nnz < 0) {
        throw std::runtime_error("CSR dimensions and nnz must be non-negative");
    }
    if (matrix.row_ptr.size() != static_cast<std::size_t>(matrix.nrows + 1)) {
        throw std::runtime_error("CSR row_ptr size does not match nrows + 1");
    }
    if (matrix.col_idx.size() != static_cast<std::size_t>(matrix.nnz) ||
        matrix.values.size() != static_cast<std::size_t>(matrix.nnz)) {
        throw std::runtime_error("CSR column/value sizes do not match nnz");
    }
    if (matrix.row_ptr.empty() || matrix.row_ptr.front() != 0 ||
        matrix.row_ptr.back() != matrix.nnz) {
        throw std::runtime_error("CSR row_ptr endpoints are invalid");
    }

    for (std::size_t i = 1; i < matrix.row_ptr.size(); ++i) {
        if (matrix.row_ptr[i] < matrix.row_ptr[i - 1]) {
            throw std::runtime_error("CSR row_ptr must be nondecreasing");
        }
    }
    for (const auto col : matrix.col_idx) {
        if (col < 0 || static_cast<std::int64_t>(col) >= matrix.ncols) {
            throw std::runtime_error("CSR column index out of bounds");
        }
    }
}

std::string matrix_name_from_path(const std::string& path) {
    const std::filesystem::path p(path);
    const auto stem = p.stem().string();
    return stem.empty() ? p.filename().string() : stem;
}

}  // namespace sparsebench
