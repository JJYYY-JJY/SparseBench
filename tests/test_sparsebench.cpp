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

void test_reject_unsupported_symmetric() {
    const auto path = std::filesystem::temp_directory_path() /
                      "sparsebench_reject_symmetric.mtx";
    {
        std::ofstream out(path);
        out << "%%MatrixMarket matrix coordinate real symmetric\n"
            << "2 2 1\n"
            << "1 1 1.0\n";
    }

    bool rejected = false;
    try {
        (void)sparsebench::read_matrix_market_to_csr(path.string());
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    std::filesystem::remove(path);
    require(rejected, "symmetric Matrix Market files must be rejected in v0.1");
}

}  // namespace

int main() {
    try {
        test_diag5_parse_and_spmv();
        test_nonsym5_parse_and_spmv();
        test_duplicate_entries_are_preserved();
        test_omp_matches_serial();
        test_reject_unsupported_symmetric();
        std::cout << "sparsebench unit tests passed\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "test failure: " << ex.what() << '\n';
        return 1;
    }
}
