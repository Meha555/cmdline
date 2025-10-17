cmake_minimum_required(VERSION 3.10)
project(cmdline)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Add compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
endif()

option(CMDLINE_USE_EXCEPTIONS "Use exceptions" ON)

option(BUILD_EXAMPLES "Build examples" ON)

add_library(cmdline INTERFACE cmdline.h)
target_include_directories(cmdline INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(CMDLINE_USE_EXCEPTIONS)
    target_compile_definitions(cmdline INTERFACE CMDLINE_USE_EXCEPTIONS=1)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
