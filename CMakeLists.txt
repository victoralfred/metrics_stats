cmake_minimum_required(VERSION 3.10)
project(metrics_agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE   "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    set (VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "Vcpkg target triplet")
else()
    set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")
endif()
find_package(OpenSSL REQUIRED)  
find_package(nlohmann_json REQUIRED)
find_package(jwt-cpp REQUIRED)

add_library(metrics_agent
    src/cpu_stats.cpp
    src/mem_stats.cpp
    )
target_include_directories(metrics_agent
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
        ${nlohmann_json_INCLUDE_DIRS}
        ${jwt-cpp_INCLUDE_DIRS}
)
target_link_libraries(metrics_agent
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        jwt-cpp::jwt-cpp
)

# Include FetchContent to get GoogleTest
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Download and add GoogleTest
FetchContent_MakeAvailable(googletest)

add_executable(metrics_agent_test
    tests/cpu_stats_test.cpp
    tests/mem_stats_test.cpp
)
add_test(metrics_agent_test
    tesst/cpu_stats_test.cpp
    tests/mem_stats_test.cpp
)
target_include_directories(metrics_agent_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Optionally link to your test framework, e.g. GoogleTest
target_link_libraries(metrics_agent_test PRIVATE metrics_agent gtest gtest_main)
# Enable test discovery
include(GoogleTest)
gtest_discover_tests(metrics_agent_test)