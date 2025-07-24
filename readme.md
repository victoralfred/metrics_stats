# System Stats Agent

This project provides a Python interface for retrieving system statistics such as CPU, memory, network, and disk usage. It uses C++ bindings for efficient data collection.

## Features
- Get CPU usage (user, system, idle)
- Get memory stats (total, available)
- Get network stats (transmit/receive rates and bytes)
- Get disk stats (read/write bytes)

## Project Structure

```
include/         # C++ header files
src/             # C++ source files and Python bindings
main.py          # Python example usage
requirements.txt # Python dependencies
setup.py         # Python package setup
Dockerfile       # Containerization
CMakeLists.txt   # C++ build configuration
tests/           # C++ unit tests
```

## Usage

1. Build the C++ extension and install the Python package:
    ```powershell
    pip install .
    ```

2. Run the example:
    ```powershell
    python main.py
    ```

## Example

```python
import sysstats

cpu = sysstats.get_cpu_stats()
print(cpu.user, cpu.system, cpu.idle)

mem = sysstats.get_mem_stats()
print(mem.total, mem.available)

network = sysstats.get_net_stats()
print(network.tx_rate, network.rx_rate)
print(network.tx_bytes, network.rx_bytes)

disk = sysstats.get_disk_stats()
print(disk.read_bytes, disk.write_bytes)
```

## Development

- C++ code is in `src/` and `include/`
- Python bindings are in `src/binding.cpp`
- Tests are in `tests/`

## License

MIT
