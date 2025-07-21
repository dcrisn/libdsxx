cmake_minimum_required(VERSION 3.5)

project( libdsxx
    LANGUAGES CXX
    VERSION 0.0.1
    DESCRIPTION "C++ >=20 data structures library"
)

set(CMAKE_CXX_STANDARD 20 )
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# make Makefiles spit out verbose output by default
set(CMAKE_VERBOSE_MAKEFILE ON)

# Generate compile-commands.json required for clangd language server.
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall
    -Wextra
    -Wuninitialized
    -Wshadow
    -Wpedantic
    -Wnull-dereference
    -fwrapv
    -fstack-protector-all
    -fstrict-aliasing
    -Wstrict-aliasing
    -fdiagnostics-color=always
    -Wno-format-truncation
    -Wold-style-cast
)

add_definitions(
    -D_POSIX_C_SOURCE=200809L
)

option(DEBUG "Build debug binaries" OFF)
option(USE_SANITIZERS "Use program sanitizers" OFF)
option(BUILD_TESTS "Generate test targets" OFF)

##################################
#        dependencies
##################################
include(cmake/dependencies.cmake)

#################
#   packaging   #
#################
include(cmake/package.cmake)

#

add_subdirectory(src)

if (BUILD_TESTS)
    message("Build will include tests")
    add_subdirectory(tests)
endif()

