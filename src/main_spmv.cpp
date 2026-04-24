#include "sparsebench/matrix_market.hpp"
#include "sparsebench/spmv.hpp"
#include "sparsebench/timer.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string matrix_path;
    std::string out_path;
    int threads = 1;
    int repeat = 10;
};

[[noreturn]] void usage(const char* program) {
    std::cerr << "usage: " << program
              << " <matrix.mtx> --threads N --repeat R --out results.csv\n";
    std::exit(2);
}

int parse_positive_int(const std::string& value, const std::string& name) {
    std::size_t pos = 0;
    const int parsed = std::stoi(value, &pos);
    if (pos != value.size() || parsed <= 0) {
        throw std::runtime_error(name + " must be a positive integer");
    }
    return parsed;
}

Options parse_args(int argc, char** argv) {
    if (argc < 2) {
        usage(argv[0]);
    }

    Options options;
    options.matrix_path = argv[1];

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            options.threads = parse_positive_int(argv[++i], "--threads");
        } else if (arg == "--repeat" && i + 1 < argc) {
            options.repeat = parse_positive_int(argv[++i], "--repeat");
        } else if (arg == "--out" && i + 1 < argc) {
            options.out_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
        } else {
            throw std::runtime_error("Unknown or incomplete argument: " + arg);
        }
    }

    if (options.out_path.empty()) {
        throw std::runtime_error("--out is required");
    }
    return options;
}

double median_ms(std::vector<double> values) {
    if (values.empty()) {
        throw std::runtime_error("Cannot compute median of empty sample set");
    }
    std::sort(values.begin(), values.end());
    const auto mid = values.size() / 2;
    if (values.size() % 2 == 1) {
        return values[mid];
    }
    return (values[mid - 1] + values[mid]) / 2.0;
}

void ensure_parent_directory(const std::string& out_path) {
    const std::filesystem::path path(out_path);
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

void assert_same_result(const std::vector<double>& expected,
                        const std::vector<double>& actual) {
    if (expected.size() != actual.size()) {
        throw std::runtime_error("SpMV result size mismatch");
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (std::abs(expected[i] - actual[i]) > 1e-9) {
            throw std::runtime_error("OpenMP SpMV result differs from serial reference");
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse_args(argc, argv);
        const auto matrix = sparsebench::read_matrix_market_to_csr(options.matrix_path);
        const std::vector<double> x(static_cast<std::size_t>(matrix.ncols), 1.0);

        std::vector<double> reference;
        sparsebench::spmv_serial(matrix, x, reference);

        std::vector<double> y;
        if (options.threads == 1) {
            sparsebench::spmv_serial(matrix, x, y);
        } else {
            sparsebench::spmv_omp(matrix, x, y, options.threads);
        }
        assert_same_result(reference, y);

        std::vector<double> samples;
        samples.reserve(static_cast<std::size_t>(options.repeat));
        for (int i = 0; i < options.repeat; ++i) {
            sparsebench::Timer timer;
            if (options.threads == 1) {
                sparsebench::spmv_serial(matrix, x, y);
            } else {
                sparsebench::spmv_omp(matrix, x, y, options.threads);
            }
            samples.push_back(timer.elapsed_ms());
        }

        const auto median = median_ms(samples);
        const auto min_ms = *std::min_element(samples.begin(), samples.end());
        const auto nnz_per_sec =
            median > 0.0 ? static_cast<double>(matrix.nnz) / (median / 1000.0) : 0.0;
        const auto final_checksum = sparsebench::checksum(y);

        ensure_parent_directory(options.out_path);
        std::ofstream out(options.out_path);
        if (!out) {
            throw std::runtime_error("Could not open CSV output path: " + options.out_path);
        }

        out << "matrix,nrows,ncols,nnz,threads,repeat,median_ms,min_ms,nnz_per_sec,checksum\n";
        out << sparsebench::matrix_name_from_path(options.matrix_path) << ','
            << matrix.nrows << ','
            << matrix.ncols << ','
            << matrix.nnz << ','
            << options.threads << ','
            << options.repeat << ','
            << std::setprecision(12) << median << ','
            << min_ms << ','
            << nnz_per_sec << ','
            << final_checksum << '\n';

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "sparsebench_spmv: " << ex.what() << '\n';
        return 1;
    }
}
