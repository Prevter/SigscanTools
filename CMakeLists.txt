cmake_minimum_required(VERSION 3.28)
project(BindingsMapper)

set(CMAKE_CXX_STANDARD 20)

add_executable(
    BindingsMapper
    src/main.cpp
    src/scanner/scanner.cpp
    src/decompiler/arm-generator.cpp
    src/decompiler/decompiler.cpp
)

add_executable(
    BindingsImporter
    src/importer.cpp
    src/scanner/scanner.cpp
)

include(cmake/get_cpm.cmake)

# Zydis
CPMAddPackage(
    NAME Zydis
    GITHUB_REPOSITORY zyantific/zydis
    GIT_TAG v4.1.0
    OPTIONS
        "ZYDIS_BUILD_EXAMPLES OFF"
        "ZYDIS_BUILD_TOOLS OFF"
        "ZYDIS_BUILD_TESTS OFF"
)

# Capstone
CPMAddPackage(
    NAME Capstone
    GITHUB_REPOSITORY capstone-engine/capstone
    GIT_TAG 5.0.3
    OPTIONS
        "CAPSTONE_BUILD_DIET OFF"
        "CAPSTONE_BUILD_TESTS OFF"
        "CAPSTONE_BUILD_CSTOOL OFF"
        "CAPSTONE_BUILD_SHARED OFF"
        "CAPSTONE_BUILD_STATIC ON"
)

target_link_libraries(BindingsMapper PRIVATE Zydis capstone)