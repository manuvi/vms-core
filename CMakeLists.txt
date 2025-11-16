cmake_minimum_required(VERSION 3.20)

project(vms-core
    VERSION 0.1.0
    DESCRIPTION "VMS Core Utilities Library"
    LANGUAGES C CXX
)

# Configurazione generale
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(VMS_CORE_ENABLE_COVERAGE "Enable gcov-based coverage instrumentation" OFF)

if(VMS_CORE_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    else()
        message(FATAL_ERROR "Coverage enabled but compiler ${CMAKE_CXX_COMPILER_ID} is unsupported")
    endif()
endif()

add_library(vms-core
    src/thread_base.cpp
    src/thread_worker.cpp
)

target_include_directories(vms-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()
add_subdirectory(tests)

if(VMS_CORE_ENABLE_COVERAGE)
    find_program(LCOV_EXECUTABLE lcov REQUIRED)
    find_program(GENHTML_EXECUTABLE genhtml REQUIRED)

    set(VMS_CORE_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage_report")
    set(VMS_CORE_COVERAGE_INFO "${CMAKE_BINARY_DIR}/coverage.info")
    set(VMS_CORE_COVERAGE_FILTERED "${CMAKE_BINARY_DIR}/coverage.filtered.info")

    add_custom_target(coverage
        COMMAND ${LCOV_EXECUTABLE} --rc branch_coverage=1
            --directory ${CMAKE_BINARY_DIR} --zerocounters
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${LCOV_EXECUTABLE} --rc branch_coverage=1
            --directory ${CMAKE_BINARY_DIR} --capture
            --output-file ${VMS_CORE_COVERAGE_INFO}
            --ignore-errors mismatch
        COMMAND ${LCOV_EXECUTABLE} --rc branch_coverage=1
            --extract ${VMS_CORE_COVERAGE_INFO}
            "${CMAKE_SOURCE_DIR}/src/*"
            "${CMAKE_SOURCE_DIR}/include/*"
            --output-file ${VMS_CORE_COVERAGE_FILTERED}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${VMS_CORE_COVERAGE_DIR}
        COMMAND ${GENHTML_EXECUTABLE}
            --branch-coverage
            --legend
            --title "vms-core coverage"
            --output-directory ${VMS_CORE_COVERAGE_DIR}
            ${VMS_CORE_COVERAGE_FILTERED}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running lcov/genhtml to generate coverage report"
    )

    add_dependencies(coverage vms-core-tests)
endif()
