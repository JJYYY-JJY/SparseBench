if(NOT DEFINED CSV)
    message(FATAL_ERROR "CSV variable is required")
endif()

if(NOT EXISTS "${CSV}")
    message(FATAL_ERROR "Expected CSV file does not exist: ${CSV}")
endif()

file(STRINGS "${CSV}" lines)
list(LENGTH lines line_count)
if(NOT line_count EQUAL 2)
    message(FATAL_ERROR "Expected exactly 2 CSV lines, found ${line_count}")
endif()

list(GET lines 0 header)
set(expected_header "matrix,nrows,ncols,nnz,threads,repeat,median_ms,min_ms,nnz_per_sec,checksum")
if(NOT header STREQUAL expected_header)
    message(FATAL_ERROR "Unexpected CSV header: ${header}")
endif()

list(GET lines 1 row)
if(NOT row MATCHES "^diag5,5,5,5,1,3,")
    message(FATAL_ERROR "Unexpected CSV row: ${row}")
endif()
