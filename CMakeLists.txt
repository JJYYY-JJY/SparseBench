cmake_minimum_required(VERSION 3.16)
project(SparseBenchPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SPARSEBENCH_USE_OPENMP "Use OpenMP for parallel kernels" ON)

add_library(sparsebench_core
    src/csr.cpp
    src/matrix_market.cpp
    src/spmv.cpp
)

target_include_directories(sparsebench_core PUBLIC include)

if(MSVC)
    target_compile_options(sparsebench_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(sparsebench_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SPARSEBENCH_USE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(sparsebench_core PUBLIC OpenMP::OpenMP_CXX)
        target_compile_definitions(sparsebench_core PUBLIC SPARSEBENCH_HAS_OPENMP=1)
    endif()
endif()

add_executable(sparsebench_spmv src/main_spmv.cpp)
target_link_libraries(sparsebench_spmv PRIVATE sparsebench_core)

if(MSVC)
    target_compile_options(sparsebench_spmv PRIVATE /W4 /permissive-)
else()
    target_compile_options(sparsebench_spmv PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(sparsebench_tests tests/test_sparsebench.cpp)
target_link_libraries(sparsebench_tests PRIVATE sparsebench_core)
target_compile_definitions(sparsebench_tests PRIVATE
    SPARSEBENCH_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(sparsebench_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(sparsebench_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME sparsebench_unit_tests COMMAND sparsebench_tests)

set(CLI_TEST_CSV "${CMAKE_CURRENT_BINARY_DIR}/cli_diag5.csv")
add_test(
    NAME cli_diag5_csv
    COMMAND sparsebench_spmv
        "${CMAKE_CURRENT_SOURCE_DIR}/data/tiny/diag5.mtx"
        --threads 1
        --repeat 3
        --out "${CLI_TEST_CSV}"
)
add_test(
    NAME cli_diag5_csv_shape
    COMMAND "${CMAKE_COMMAND}"
        "-DCSV=${CLI_TEST_CSV}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli_csv.cmake"
)
set_tests_properties(cli_diag5_csv_shape PROPERTIES DEPENDS cli_diag5_csv)
