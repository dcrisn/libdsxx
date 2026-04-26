cmake_minimum_required(VERSION 3.25)

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

if (DEBUG)
    message("This is a *DEBUG* build")
    add_compile_options(-ggdb3)
    add_definitions(-DDEBUG_BUILD)
else()
    message("This is a *PRODUCTION* build")
    add_compile_options(-O3)
    #add_compile_options(-Werror)
    #add_definitions(-DNDEBUG)  # strip out asserts
endif()

if (USE_SANITIZERS)
    message("Trying to build with sanitizer support")
    foreach(flag
            -fsanitize=address;
            -fsanitize=leak;
            -fsanitize=undefined; # includes null, alignment, bounds-strict etc
            -fsanitize=pointer-compare;
            -fsanitize=pointer-subtract;
            #-fsanitize=thread
            )
        message("sanitizer flag: ${flag}")

        # TODO: find way to check if san libs exist / compiler supports the flags
        SET(has_flag 1)

        if (has_flag)
            add_compile_options(${flag})
            add_link_options(${flag})
        else()
            message(NOTICE "Failed to add sanitizer flag '${flag}' (unacceptable)")
        endif()

    endforeach()
endif()


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

