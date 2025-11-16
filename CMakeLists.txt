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
