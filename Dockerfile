# === Builder stage ===
FROM ubuntu:22.04 AS builder

ENV TZ=Etc/UTC
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies and Python 3.9 in one go
RUN apt-get update && \
    apt-get install -y software-properties-common build-essential cmake git wget curl zip unzip tar g++ clang pkg-config && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update && \
    apt-get install -y python3.9 python3.9-dev python3.9-venv python3-pip && \
    rm -rf /var/lib/apt/lists/*

# Setup Python 3.9 venv and upgrade pip tools
RUN python3.9 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

RUN pip install --upgrade pip setuptools wheel pybind11

# Install vcpkg and required C++ dependencies
RUN git clone https://github.com/microsoft/vcpkg.git /vcpkg
WORKDIR /vcpkg
RUN ./bootstrap-vcpkg.sh
RUN ./vcpkg install openssl nlohmann-json jwt-cpp picojson

ENV VCPKG_ROOT=/vcpkg
ENV PATH="${VCPKG_ROOT}:$PATH"
ENV CMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
ENV CMAKE_BUILD_TYPE=Release

WORKDIR /workdir

# Copy CMakeLists.txt first to leverage docker cache for dependencies
COPY CMakeLists.txt /workdir/
# Copy source and headers after configuration for better caching
COPY src/ /workdir/src
COPY include/ /workdir/include
COPY tests/ /workdir/tests

# Configure cmake (empty build directory)
RUN mkdir build && \
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}



# Build project and run tests
RUN cmake --build build --config ${CMAKE_BUILD_TYPE}
RUN ctest --output-on-failure --test-dir build

# Copy setup.py and build Python extension in-place
COPY setup.py /workdir/
RUN python setup.py build_ext --inplace

# Copy main.py
COPY main.py /workdir/

# Build wheel and install it in venv for testing
RUN python setup.py bdist_wheel
RUN pip install dist/*.whl

# === Runtime stage ===
FROM python:3.9-slim AS runtime

# Install runtime dependencies (openssl library for example)
RUN apt-get update && apt-get install -y libssl-dev && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built artifacts and python files from builder stage
COPY --from=builder /workdir/build/ /app/build/
COPY --from=builder /workdir/*.so /app/
COPY --from=builder /workdir/*.py /app/

# Optional: install pybind11 if your extension requires it at runtime, else remove this
RUN pip install --no-cache-dir pybind11

ENV PYTHONPATH=/app

CMD ["python3.9", "main.py"]
