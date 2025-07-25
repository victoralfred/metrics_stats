#!/bin/bash

# --- 1. Environment Setup ---
# Set VCPKG_ROOT if it's not already defined (or if you prefer a consistent path)
export VCPKG_ROOT="${HOME}/vcpkg"
# Add VCPKG to PATH if the vcpkg command is needed directly
export PATH="${VCPKG_ROOT}:$PATH"
# Set CMake Toolchain File for VCPKG integration
export CMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
# Set default build type (can be overridden by CMake command line if needed)
export CMAKE_BUILD_TYPE="Release" # Or "Debug" if preferred for development

echo "VCPKG_ROOT set to: ${VCPKG_ROOT}"
echo "CMAKE_TOOLCHAIN_FILE set to: ${CMAKE_TOOLCHAIN_FILE}"
echo "CMAKE_BUILD_TYPE set to: ${CMAKE_BUILD_TYPE}"

# --- 2. Install Dependencies with VCPKG ---
echo "Installing VCPKG dependencies..."
# Ensure you only install libraries that are actually used in CMakeLists.txt
# If picojson is not used in CMakeLists.txt, remove it from here.
vcpkg install openssl nlohmann-json jwt-cpp # Removed picojson for now, add back if used.

# Check if vcpkg install was successful
if [ $? -ne 0 ]; then
    echo "Error: VCPKG dependency installation failed. Exiting."
    exit 1
fi

# --- 3. Clean and Create Build Directory ---
# Ensure you are at the project root where CMakeLists.txt is located
echo "Cleaning and creating build directory..."
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

rm -rf build
if [ -d "build" ]; then
    echo "Error: Failed to remove existing build directory. Please check permissions."
    exit 1
fi
echo "Creating build directory..."
# Create the build directory
mkdir build

# --- 4. Configure and Build with CMake ---
echo "Configuring project with CMake..."
cd build &&  cmake .. \
     -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}"  \
     -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"  \
     -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
     -DENABLE_TESTING=ON # Enable testing if needed

# Check if cmake configuration was successful
if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed. Exiting."
    exit 1
fi 

echo "Building project with Make..."
make # Run make from within the build directory

# Check if make build was successful
if [ $? -ne 0 ]; then
    echo "Error: Make build failed. Exiting."
    exit 1
fi

echo "Build process completed successfully!"

# --- Optional: Run Tests ---
# echo "Running tests..." 
# ctest --verbose         
# if [ $? -ne 0 ]; then    
#     echo "Error: Tests failed."
#     exit 1
# fi                       

echo "Build process completed successfully!"