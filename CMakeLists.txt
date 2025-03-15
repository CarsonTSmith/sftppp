cmake_minimum_required(VERSION 3.16)
project(sftpclientpp CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/target)

# Enable strict warnings and treat warnings as errors
add_compile_options(
    -Wall                 # Enable all warnings
    -Wextra               # Extra warnings
    -Wpedantic            # Strict standard compliance
    -Werror               # Treat warnings as errors
    -Wshadow              # Warn if a variable shadows another
    -Wuninitialized       # Warn about uninitialized variables
    -Wconversion          # Warn about implicit type conversions
    -Wdouble-promotion    # Warn about float -> double promotions
    -Wcast-align          # Warn about invalid pointer type casts
    -Wpointer-arith       # Warn about invalid pointer arithmetic
    -Wlogical-op          # Warn about ambiguous logical operations
    -Wnull-dereference    # Warn about dereferencing null pointers
)

# Enable Clang-Format if available
find_program(CLANG_FORMAT NAMES clang-format)
if(CLANG_FORMAT)
    add_custom_target(
        format
        COMMAND find ${CMAKE_SOURCE_DIR}/src -name "*.cpp" -o -name "*.h" | xargs ${CLANG_FORMAT} -i
        COMMENT "Formatting code with clang-format"
    )
endif()

# Collect all source files
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)

# Build as a shared library
add_library(sftpclientpp SHARED ${SOURCES})

# Link against libssh
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSSH REQUIRED libssh)
target_include_directories(sftpclientpp PRIVATE ${LIBSSH_INCLUDE_DIRS})
target_link_libraries(sftpclientpp ${LIBSSH_LIBRARIES})

# Automatically run clang-format before building the library
add_dependencies(sftpclientpp format)