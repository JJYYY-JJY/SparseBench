#include "sparsebench/matrix_market.hpp"
#include "sparsebench/spmv.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

std::filesystem::path data_path(const std::string& name) {
    return std::filesystem::path(SPARSEBENCH_SOURCE_DIR) / "data" / "tiny" / name;
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_near(double actual, double expected, const std::string& message) {
    if (std::abs(actual - expected) > 1e-12) {
        throw std::runtime_error(message);
    }
}

void require_vector_near(const std::vector<double>& actual,
                         const std::vector<double>& expected,
                         const std::string& message) {
    require(actual.size() == expected.size(), message + ": size mismatch");
    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (std::abs(actual[i] - expected[i]) > 1e-12) {
            throw std::runtime_error(message);
        }
    }
}

void test_diag5_parse_and_spmv() {
    const auto matrix =
        sparsebench::read_matrix_market_to_csr(data_path("diag5.mtx").string());

    require(matrix.nrows == 5, "diag5 nrows");
    require(matrix.ncols == 5, "diag5 ncols");
    require(matrix.nnz == 5, "diag5 nnz");
    require((matrix.row_ptr == std::vector<std::int64_t>{0, 1, 2, 3, 4, 5}),
            "diag5 row_ptr");
    require((matrix.col_idx == std::vector<std::int32_t>{0, 1, 2, 3, 4}),
            "diag5 col_idx");

    const std::vector<double> x(5, 1.0);
    std::vector<double> y;
    sparsebench::spmv_serial(matrix, x, y);
    require_vector_near(y, {1.0, 2.0, 3.0, 4.0, 5.0}, "diag5 serial spmv");
    require_near(sparsebench::checksum(y), 15.0, "diag5 checksum");
}

void test_nonsym5_parse_and_spmv() {
    const auto matrix =
        sparsebench::read_matrix_market_to_csr(data_path("nonsym5.mtx").string());

    require(matrix.nrows == 5, "nonsym5 nrows");
    require(matrix.ncols == 5, "nonsym5 ncols");
    require(matrix.nnz == 7, "nonsym5 nnz");
    require((matrix.row_ptr == std::vector<std::int64_t>{0, 2, 3, 4, 5, 7}),
            "nonsym5 row_ptr");

    const std::vector<double> x(5, 1.0);
    std::vector<double> y;
    sparsebench::spmv_serial(matrix, x, y);
    require_vector_near(y, {3.0, 3.0, 4.0, 5.0, 13.0}, "nonsym5 serial spmv");
    require_near(sparsebench::checksum(y), 28.0, "nonsym5 checksum");
}

void test_duplicate_entries_are_preserved() {
    const auto matrix =
        sparsebench::read_matrix_market_to_csr(data_path("duplicate5.mtx").string());

    require(matrix.nrows == 5, "duplicate5 nrows");
    require(matrix.ncols == 5, "duplicate5 ncols");
    require(matrix.nnz == 6, "duplicate5 nnz preserves declared entries");
    require((matrix.row_ptr == std::vector<std::int64_t>{0, 2, 3, 4, 5, 6}),
            "duplicate5 row_ptr preserves duplicate entries in row 1");
    require((matrix.col_idx[0] == 0 && matrix.col_idx[1] == 0),
            "duplicate5 duplicate column entries are not coalesced");

    const std::vector<double> x(5, 1.0);
    std::vector<double> y;
    sparsebench::spmv_serial(matrix, x, y);
    require_vector_near(y, {3.5, 4.0, 5.0, 6.0, 7.0},
                        "duplicate5 serial spmv sums duplicate contributions");
}

void test_integer_general_parse_and_spmv() {
    const auto matrix =
        sparsebench::read_matrix_market_to_csr(data_path("integer5.mtx").string());

    require(matrix.nrows == 5, "integer5 nrows");
    require(matrix.ncols == 5, "integer5 ncols");
    require(matrix.nnz == 5, "integer5 nnz");
    require((matrix.row_ptr == std::vector<std::int64_t>{0, 1, 2, 3, 4, 5}),
            "integer5 row_ptr");
    require((matrix.col_idx == std::vector<std::int32_t>{0, 1, 2, 3, 4}),
            "integer5 col_idx");

    const std::vector<double> x(5, 1.0);
    std::vector<double> y;
    sparsebench::spmv_serial(matrix, x, y);
    require_vector_near(y, {1.0, 2.0, 3.0, 4.0, 5.0}, "integer5 serial spmv");
    require_near(sparsebench::checksum(y), 15.0, "integer5 checksum");
}

void test_pattern_general_parse_and_spmv() {
    const auto matrix =
        sparsebench::read_matrix_market_to_csr(data_path("pattern5.mtx").string());

    require(matrix.nrows == 5, "pattern5 nrows");
    require(matrix.ncols == 5, "pattern5 ncols");
    require(matrix.nnz == 6, "pattern5 nnz");
    require((matrix.row_ptr == std::vector<std::int64_t>{0, 2, 3, 4, 5, 6}),
            "pattern5 row_ptr");
    require((matrix.col_idx == std::vector<std::int32_t>{0, 2, 1, 0, 4, 3}),
            "pattern5 col_idx");

    const std::vector<double> x(5, 1.0);
    std::vector<double> y;
    sparsebench::spmv_serial(matrix, x, y);
    require_vector_near(y, {2.0, 1.0, 1.0, 1.0, 1.0}, "pattern5 serial spmv");
    require_near(sparsebench::checksum(y), 6.0, "pattern5 checksum");
}

void test_symmetric_real_expands_mirror_entries() {
    const auto matrix =
        sparsebench::read_matrix_market_to_csr(data_path("symmetric5.mtx").string());

    require(matrix.nrows == 5, "symmetric5 nrows");
    require(matrix.ncols == 5, "symmetric5 ncols");
    require(matrix.nnz == 6, "symmetric5 expanded nnz");
    require((matrix.row_ptr == std::vector<std::int64_t>{0, 2, 3, 4, 5, 6}),
            "symmetric5 row_ptr");
    require((matrix.col_idx == std::vector<std::int32_t>{0, 2, 4, 0, 3, 1}),
            "symmetric5 col_idx");

    const std::vector<double> x(5, 1.0);
    std::vector<double> y;
    sparsebench::spmv_serial(matrix, x, y);
    require_vector_near(y, {7.0, 7.0, 5.0, 11.0, 7.0}, "symmetric5 serial spmv");
    require_near(sparsebench::checksum(y), 37.0, "symmetric5 checksum");
}

void test_omp_matches_serial() {
    const auto matrix =
        sparsebench::read_matrix_market_to_csr(data_path("nonsym5.mtx").string());
    const std::vector<double> x(5, 1.0);
    std::vector<double> reference;
    sparsebench::spmv_serial(matrix, x, reference);

    std::vector<int> thread_counts{1, 2};
    const auto hardware = std::thread::hardware_concurrency();
    if (hardware > 2) {
        thread_counts.push_back(static_cast<int>(std::min<unsigned int>(hardware, 4)));
    }

    for (const int threads : thread_counts) {
        std::vector<double> y;
        sparsebench::spmv_omp(matrix, x, y, threads);
        require_vector_near(y, reference, "OpenMP result must match serial");
    }
}

void require_rejects(const std::string& name,
                     const std::string& contents,
                     const std::string& message) {
    const auto path = std::filesystem::temp_directory_path() / name;
    {
        std::ofstream out(path);
        out << contents;
    }

    bool rejected = false;
    try {
        (void)sparsebench::read_matrix_market_to_csr(path.string());
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    std::filesystem::remove(path);
    require(rejected, message);
}

void test_reject_unsupported_headers() {
    require_rejects(
        "sparsebench_reject_complex.mtx",
        "%%MatrixMarket matrix coordinate complex general\n"
        "2 2 1\n"
        "1 1 1.0 0.0\n",
        "complex Matrix Market files must be rejected");

    require_rejects(
        "sparsebench_reject_hermitian.mtx",
        "%%MatrixMarket matrix coordinate real Hermitian\n"
        "2 2 1\n"
        "1 1 1.0\n",
        "Hermitian Matrix Market files must be rejected");

    require_rejects(
        "sparsebench_reject_skew_symmetric.mtx",
        "%%MatrixMarket matrix coordinate real skew-symmetric\n"
        "2 2 1\n"
        "1 2 1.0\n",
        "skew-symmetric Matrix Market files must be rejected");

    require_rejects(
        "sparsebench_reject_array.mtx",
        "%%MatrixMarket matrix array real general\n"
        "2 2\n"
        "1.0\n"
        "2.0\n"
        "3.0\n"
        "4.0\n",
        "array Matrix Market files must be rejected");
}

}  // namespace

int main() {
    try {
        test_diag5_parse_and_spmv();
        test_nonsym5_parse_and_spmv();
        test_duplicate_entries_are_preserved();
        test_integer_general_parse_and_spmv();
        test_pattern_general_parse_and_spmv();
        test_symmetric_real_expands_mirror_entries();
        test_omp_matches_serial();
        test_reject_unsupported_headers();
        std::cout << "sparsebench unit tests passed\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "test failure: " << ex.what() << '\n';
        return 1;
    }
}
