# Minimum CMake version required for features like FetchContent and modern Python finding.
cmake_minimum_required(VERSION 3.28) # Good version, ensures modern CMake features are available

# Define your project and specify C++ as the primary language.
project(metrics_agent LANGUAGES CXX)

# --- C++ Standard Settings ---
set(CMAKE_CXX_STANDARD 17) # Use C++17 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Enforce C++17
set(CMAKE_CXX_EXTENSIONS OFF) # Do not allow GNU extensions (for better portability)

# --- Build Configuration ---
set(CMAKE_BUILD_TYPE Debug) # Set build type (Debug, Release, RelWithDebInfo, MinSizeRel)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json for IDEs (e.g., VS Code)

# --- Python3 and Pybind11 Integration ---

# Find the specific Python3 version.
# If you explicitly need 3.6 again, change this to '3.6'.
# This configuration will find Python 3.12.3 as per your last successful find.
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Python3 Interpreter: ${Python3_EXECUTABLE}")
message(STATUS "Python3 Include Dir: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3 Libraries: ${Python3_LIBRARIES}")
message(STATUS "Python3 Version: ${Python3_VERSION}")

# Force pybind11 to use modern FindPython3
set(PYBIND11_FINDPYTHON ON CACHE BOOL "Use modern FindPython3 in pybind11" FORCE)


# REMOVED: set(PYBIND11_FINDPYTHON ON CACHE BOOL ...) from here.
# This flag should be passed via CMAKE_ARGS to FetchContent_Declare, not set globally.

# Fetch Pybind11 as an external project (modern CMake build)
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.12.0 # Use a stable release tag
)
FetchContent_MakeAvailable(pybind11) # Makes pybind11's targets available in your project

# Fetch GoogleTest (moved here for better logical flow before its usage)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  # Optionally, specify a specific commit/tag with GIT_TAG if URL becomes unstable
  # GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest) # Make googletest's targets available

# --- VCPKG Setup ---
# Handles finding and integrating vcpkg for dependency management.
if(DEFINED ENV{VCPKG_ROOT}) # Check if VCPKG_ROOT environment variable is set
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    set (VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "Vcpkg target triplet")
else()
    # If VCPKG_ROOT is not set, assume vcpkg is a submodule or located directly in source dir
    set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")
    if (EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        # Use FORCE to override cache in case it was set incorrectly earlier
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)
        set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "Vcpkg target triplet" FORCE)
    else()
        message(WARNING "VCPKG_ROOT environment variable not set and vcpkg submodule not found at ${VCPKG_ROOT}. Dependency resolution might fail.")
    endif()
endif()

# --- Find External Dependencies (managed by VCPKG) ---
# These are found via vcpkg's integration through CMAKE_TOOLCHAIN_FILE.
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(jwt-cpp REQUIRED)

# --- Define Your Core C++ Library ---
add_library(metrics_agent STATIC # Define as STATIC library
    src/cpu_stats.cpp
    src/mem_stats.cpp
    src/disk_stats.cpp
    src/net_stats.cpp
    # Add all other source files that are part of your core C++ library here
)

# CRITICAL FIX: Ensure the static library is compiled with Position-Independent Code (-fPIC).
# This is absolutely necessary when linking a static library into a shared object (like your Python module).
set_target_properties(metrics_agent PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Specify include directories for your core C++ library.
target_include_directories(metrics_agent
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include # Your project's own headers (e.g., SystemCPUStats.h)
        ${OPENSSL_INCLUDE_DIR} # Headers for OpenSSL
        ${nlohmann_json_INCLUDE_DIRS} # Headers for nlohmann/json
        ${jwt-cpp_INCLUDE_DIRS} # Headers for jwt-cpp
        # REMOVED: ${googletest_SOURCE_DIR}/googletest/include from here.
        # GoogleTest headers typically belong only to test executables, not the production library itself,
        # unless your core library directly uses GTest constructs (like FRIEND_TEST), which is rare.
)

# Link external libraries to your core C++ library.
target_link_libraries(metrics_agent
    PRIVATE
        OpenSSL::SSL # Link against OpenSSL's SSL component
        OpenSSL::Crypto # Link against OpenSSL's Crypto component
        nlohmann_json::nlohmann_json # Link against nlohmann/json
        jwt-cpp::jwt-cpp # Link against jwt-cpp
)

# --- Define the Python Extension Module ---
# Ensure your 'metrics_agent' library is defined BEFORE 'py_metrics_agent' as it's a dependency.
pybind11_add_module(py_metrics_agent # This is the name your Python code will 'import'
    bindings/binding.cpp   # The C++ source file that contains your pybind11 bindings
)

# Link the Python module to your core C++ library and any direct dependencies.
# When 'py_metrics_agent' links 'metrics_agent', 'metrics_agent's PRIVATE dependencies
# (OpenSSL, nlohmann_json, jwt-cpp) are typically pulled in transitively.
target_link_libraries(py_metrics_agent PRIVATE
    metrics_agent # Link to your core C++ library
    # REMOVED: Redundant direct linking to OpenSSL, nlohmann_json, jwt-cpp.
    # These are already linked by 'metrics_agent' and will be transitively linked to 'py_metrics_agent'.
    # OpenSSL::SSL
    # OpenSSL::Crypto
    # nlohmann_json::nlohmann_json
    # jwt-cpp::jwt-cpp
)

# Ensure the binding module can find your project's headers (like SystemCPUStats.h)
# and Python's own headers.
target_include_directories(py_metrics_agent PRIVATE
    ${CMAKE_SOURCE_DIR}/include # Your project's headers needed by bindings
    ${Python3_INCLUDE_DIRS} # Python's headers needed by pybind11
)

# --- GoogleTest Configuration for Tests ---
# Prevent GoogleTest from overriding the parent project's compiler/linker settings,
# especially relevant on Windows for consistent Runtime Library linkage.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Define the test executable
add_executable(metrics_agent_test
    tests/metric_agent_stats_test.cpp
)
target_compile_definitions(metrics_agent_test PRIVATE TESTING_BUILD) # Define a macro for test-specific code

# Link the test executable to your library and GoogleTest framework.
target_link_libraries(metrics_agent_test PRIVATE metrics_agent gtest gtest_main)

# IMPORTANT: Add GoogleTest include directories for your test executable
target_include_directories(metrics_agent_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include # Your project headers needed by tests
    ${googletest_SOURCE_DIR}/googletest/include # GoogleTest framework headers
    # If your source files (e.g., cpu_stats.cpp) *must* include headers from src/
    # that are not in include/, you might need ${CMAKE_SOURCE_DIR}/src here,
    # but generally, all headers meant for public use should be in 'include/'.
)

# Enable test discovery for CTest (CMake's testing driver)
enable_testing()
include(GoogleTest) # This CMake module provides gtest_discover_tests
gtest_discover_tests(metrics_agent_test) # Automatically creates test entries for CTest

# --- Installation Rules ---
# Install the compiled Python module into the Python site-packages directory.
install(TARGETS py_metrics_agent
    LIBRARY DESTINATION ${Python3_SITEARCH}  # For shared libraries on Linux/macOS
    RUNTIME DESTINATION ${Python3_SITEARCH}  # For DLLs on Windows (often same as LIB)
)