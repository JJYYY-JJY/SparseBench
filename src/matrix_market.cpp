#include "sparsebench/matrix_market.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace sparsebench {
namespace {

struct CooEntry {
    std::int64_t row = 0;
    std::int32_t col = 0;
    double value = 0.0;
};

struct MatrixMarketHeader {
    bool pattern = false;
    bool symmetric = false;
};

std::string lower_copy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

bool next_data_line(std::istream& input, std::string& line) {
    while (std::getline(input, line)) {
        const auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }
        if (line[first] == '%') {
            continue;
        }
        return true;
    }
    return false;
}

MatrixMarketHeader parse_header(const std::string& line, const std::string& path) {
    std::istringstream iss(line);
    std::string banner;
    std::string object;
    std::string format;
    std::string field;
    std::string symmetry;
    std::string extra;

    if (!(iss >> banner >> object >> format >> field >> symmetry) || (iss >> extra)) {
        throw std::runtime_error("Malformed Matrix Market header in " + path);
    }

    banner = lower_copy(banner);
    object = lower_copy(object);
    format = lower_copy(format);
    field = lower_copy(field);
    symmetry = lower_copy(symmetry);

    if (banner != "%%matrixmarket" || object != "matrix" ||
        format != "coordinate") {
        throw std::runtime_error(
            "Only Matrix Market coordinate matrices are supported: " + path);
    }

    if (field == "real" && symmetry == "general") {
        return MatrixMarketHeader{};
    }
    if (field == "integer" && symmetry == "general") {
        return MatrixMarketHeader{};
    }
    if (field == "pattern" && symmetry == "general") {
        return MatrixMarketHeader{true, false};
    }
    if (field == "real" && symmetry == "symmetric") {
        return MatrixMarketHeader{false, true};
    }

    throw std::runtime_error(
        "Unsupported Matrix Market header in v0.1.1: " + path);
}

CSRMatrix build_csr(std::int64_t nrows,
                    std::int64_t ncols,
                    std::vector<CooEntry> entries) {
    CSRMatrix matrix;
    matrix.nrows = nrows;
    matrix.ncols = ncols;
    matrix.nnz = static_cast<std::int64_t>(entries.size());
    matrix.row_ptr.assign(static_cast<std::size_t>(nrows + 1), 0);
    matrix.col_idx.assign(entries.size(), 0);
    matrix.values.assign(entries.size(), 0.0);

    for (const auto& entry : entries) {
        ++matrix.row_ptr[static_cast<std::size_t>(entry.row + 1)];
    }
    for (std::size_t i = 1; i < matrix.row_ptr.size(); ++i) {
        matrix.row_ptr[i] += matrix.row_ptr[i - 1];
    }

    auto next = matrix.row_ptr;
    for (const auto& entry : entries) {
        const auto row = static_cast<std::size_t>(entry.row);
        const auto dest = static_cast<std::size_t>(next[row]++);
        matrix.col_idx[dest] = entry.col;
        matrix.values[dest] = entry.value;
    }

    validate_csr(matrix);
    return matrix;
}

}  // namespace

CSRMatrix read_matrix_market_to_csr(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open Matrix Market file: " + path);
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("Empty Matrix Market file: " + path);
    }
    const auto header = parse_header(line, path);

    if (!next_data_line(input, line)) {
        throw std::runtime_error("Missing Matrix Market size line: " + path);
    }

    std::int64_t nrows = 0;
    std::int64_t ncols = 0;
    std::int64_t declared_nnz = 0;
    {
        std::istringstream dims(line);
        std::string extra;
        if (!(dims >> nrows >> ncols >> declared_nnz) || (dims >> extra)) {
            throw std::runtime_error("Malformed Matrix Market size line: " + path);
        }
    }

    if (nrows <= 0 || ncols <= 0 || declared_nnz < 0) {
        throw std::runtime_error("Invalid Matrix Market dimensions: " + path);
    }
    if (header.symmetric && nrows != ncols) {
        throw std::runtime_error("Symmetric Matrix Market files must be square: " + path);
    }
    if (ncols > std::numeric_limits<std::int32_t>::max()) {
        throw std::runtime_error("Matrix has too many columns for int32 col_idx: " + path);
    }

    std::vector<CooEntry> entries;
    entries.reserve(static_cast<std::size_t>(
        declared_nnz * (header.symmetric ? 2 : 1)));

    std::int64_t file_entries = 0;
    while (next_data_line(input, line)) {
        std::int64_t row = 0;
        std::int64_t col = 0;
        double value = 1.0;
        std::string extra;
        std::istringstream entry_line(line);
        if (header.pattern) {
            if (!(entry_line >> row >> col) || (entry_line >> extra)) {
                throw std::runtime_error("Malformed Matrix Market pattern entry: " + path);
            }
        } else {
            if (!(entry_line >> row >> col >> value) || (entry_line >> extra)) {
                throw std::runtime_error("Malformed Matrix Market entry: " + path);
            }
        }
        if (row < 1 || row > nrows || col < 1 || col > ncols) {
            throw std::runtime_error("Matrix Market entry index out of bounds: " + path);
        }
        entries.push_back(CooEntry{
            row - 1,
            static_cast<std::int32_t>(col - 1),
            value,
        });
        if (header.symmetric && row != col) {
            entries.push_back(CooEntry{
                col - 1,
                static_cast<std::int32_t>(row - 1),
                value,
            });
        }
        ++file_entries;
    }

    if (file_entries != declared_nnz) {
        throw std::runtime_error("Matrix Market entry count does not match header: " + path);
    }

    return build_csr(nrows, ncols, std::move(entries));
}

}  // namespace sparsebench
