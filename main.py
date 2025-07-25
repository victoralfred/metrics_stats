import sys
import time
import platform
import os # Added for path handling

if sys.version_info < (3, 6):
    raise RuntimeError("This module requires Python 3.6 or higher.")

# Import the py_metrics_agent module (renamed to py_metrics_agent)
try:
    # Adjust sys.path to find your compiled module
    # (Assuming 'build' is your CMake build directory at the project root)
    build_dir = os.path.join(os.path.dirname(__file__), 'build')
    if os.path.exists(os.path.join(build_dir, 'Release')):
        sys.path.append(os.path.join(build_dir, 'Release'))
    elif os.path.exists(os.path.join(build_dir, 'Debug')):
        sys.path.append(os.path.join(build_dir, 'Debug'))
    else:
        sys.path.append(build_dir)

    import py_metrics_agent as metrics # Changed import name to match C++ PYBIND11_MODULE
    print(f"Successfully imported py_metrics_agent from: {metrics.__file__}")
except ImportError as e:
    raise RuntimeError(f"Failed to import py_metrics_agent. Ensure it is built and installed correctly. Error: {e}") from e

print("--- Testing CPUStats object ---")
try:
    # CPUStats constructor: (user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice, usage_percent)
    # Default usage_percent is 0.0 for manual instantiation, it's calculated by `calculate_usage_percentage`
    stats1 = metrics.CPUStats(user=100, nice=0, system=50, idle=850, iowait=0, irq=0, softirq=0, steal=0, guest=0, guest_nice=0)
    print(f"Initial stats: {stats1}")
    print(f"Total active time: {stats1.get_total_active_time()}")
    print(f"Total time: {stats1.get_total_time()}")

    # Accessing and modifying members
    stats1.user += 10
    print(f"Stats after user activity: {stats1}")
except AttributeError as e:
    print(f"Error accessing CPUStats attributes/methods: {e}. Check CPUStats binding in C++.")
except TypeError as e:
    print(f"Error creating CPUStats object: {e}. Check CPUStats constructor binding in C++.")
except Exception as e:
    print(f"An unexpected error occurred during CPUStats object test: {e}")


print("\n--- Testing metrics.get_cpu_stats() (current system stats) ---")
try:
    # This calls the C++ CPUStatsReader::getCPUStats() which reads from /proc/stat
    current_raw_stats = metrics.get_cpu_stats()
    print(f"Current raw CPU stats: {current_raw_stats}")
    print(f"Current raw total time: {current_raw_stats.get_total_time()}")
    print(f"Note: usage_percent is 0.0 for raw stats directly from reader: {current_raw_stats.usage_percent}")

except RuntimeError as e:
    print(f"Error getting current CPU stats: {e}")
except Exception as e:
    print(f"An unexpected error occurred getting current CPU stats: {e}")

print("\n--- Testing metrics.get_cpu_stats_from_string() ---")
mock_proc_stat_line = "cpu 1000 200 300 4000 50 10 5 0 0 0"
try:
    mock_stats = metrics.get_cpu_stats_from_string(mock_proc_stat_line)
    print(f"Mock CPU stats from string: {mock_stats}")
    print(f"Mock total active time: {mock_stats.get_total_active_time()}")
except Exception as e:
    print(f"Error testing get_cpu_stats_from_string: {e}")


print("\n--- Testing calculate_cpu_usage_percentage ---") # Function name update
try:
    # These values are arbitrary for testing the calculation, not based on actual /proc/stat lines
    prev_snap = metrics.CPUStats(user=1000, nice=0, system=200, idle=8800, iowait=0, irq=0, softirq=0, steal=0, guest=0, guest_nice=0) # Total 10000, Active 1200
    curr_snap = metrics.CPUStats(user=1100, nice=0, system=220, idle=9700, iowait=0, irq=0, softirq=0, steal=0, guest=0, guest_nice=0) # Total 11020, Active 1320

    time_delta_ms = 1000

    # The C++ function handles the time_delta_ms implicitly for calculating the percentage based on tick differences.
    usage_calc = metrics.calculate_cpu_usage_percentage(curr_snap, prev_snap, time_delta_ms) # Function name update
    print(f"Calculated usage percentage: {usage_calc:.2f}%")

    # Test error for non-positive time_delta_ms
    try:
        metrics.calculate_cpu_usage_percentage(curr_snap, prev_snap, 0) # Function name update
    except ValueError as e:
        print(f"Caught expected error for 0 time_delta: {e}")

except Exception as e:
    print(f"Error testing calculate_cpu_usage_percentage: {e}")


print("\n--- Testing get_cpu_usage_over_time (example utility) ---")
try:
    print("Measuring CPU usage over 1 second...")
    usage_live = metrics.get_cpu_usage_over_time(1000) # 1000 ms = 1 second
    print(f"Live CPU usage over 1 second: {usage_live:.2f}%")

    print("Measuring CPU usage over 0.5 seconds...")
    usage_live_short = metrics.get_cpu_usage_over_time(500) # 500 ms = 0.5 seconds
    print(f"Live CPU usage over 0.5 seconds: {usage_live_short:.2f}%")

except ValueError as e:
    print(f"Error: {e}. Check if interval is positive.")
except RuntimeError as e:
    print(f"System Error: {e}")
except Exception as e:
    print(f"An unexpected error occurred: {e}")

print("\n--- Testing MemoryStats and MeMStatsReader ---")
try:
    # Test MemStats object creation
    # The C++ binding for MemStats currently only supports a default constructor.
    # We create a default object and then set its members.
    mem_stats_mock = metrics.MemStats()
    mem_stats_mock.total = 16 * 1024 * 1024
    mem_stats_mock.free = 8 * 1024 * 1024
    mem_stats_mock.available = 10 * 1024 * 1024
    print(f"Mock memory stats: {mem_stats_mock}")
    print(f"Mock total memory: {mem_stats_mock.total} KB")

    # Test metrics.get_mem_stats()
    current_mem_stats = metrics.get_mem_stats()
    print(f"Current system memory stats: {current_mem_stats}")
    print(f"Total system memory: {current_mem_stats.total} KB")
    print(f"Free system memory: {current_mem_stats.free} KB")
    print(f"Available system memory: {current_mem_stats.available} KB")
    print(f"Cached memory: {current_mem_stats.cached} KB")

except AttributeError as e:
    print(f"Error accessing MemStats attributes/methods: {e}. Check MemStats binding in C++.")
except TypeError as e:
    print(f"Error creating MemStats object: {e}. Check MemStats constructor binding in C++.")
except Exception as e:
    print(f"An error occurred during memory stats testing: {e}")


print("\n--- Testing NetworkStats and NetStatsReader ---") # Renamed from NetworkStatsReader to NetStatsReader
try:
    # NetStats constructor: (interface_name, rx_bytes, rx_packets, rx_errors, rx_dropped, tx_bytes, tx_packets, tx_errors, tx_dropped)
    net_stats1 = metrics.NetStats(
        interface_name="eth0",
        bytes_received=10000, packets_received=100, errors_in=0, drops_in=0,
        bytes_sent=5000, packets_sent=50, errors_out=0, drops_out=0 # Corrected tx_errors to errors_out
    )
    print(f"Initial network stats: {net_stats1}")
    print(f"Interface: {net_stats1.interface_name}, Rx Bytes: {net_stats1.bytes_received}, Tx Bytes: {net_stats1.bytes_sent}")

    # Test metrics.get_net_stats_aggregated() (returns a single aggregated NetStats object)
    aggregated_net_stats = metrics.get_net_stats_aggregated() # Renamed from get_net_stats()
    print("\nCurrent aggregated network stats (single object):")
    if isinstance(aggregated_net_stats, metrics.NetStats):
        print(f"  Aggregated: {aggregated_net_stats}")
        print(f"  Aggregated Rx Bytes: {aggregated_net_stats.bytes_received}, Aggregated Tx Bytes: {aggregated_net_stats.bytes_sent}")
    else:
        print(f"  Expected a single NetStats object, but got: {type(aggregated_net_stats)}")


    # Test metrics.get_net_stats_by_interface() (returns a specific interface's stats)
    # The C++ binding expects a single interface name.
    print("\nAttempting to get stats for loopback interface:")
    loopback_name = "lo" # Linux default
    if platform.system() == 'Windows':
        loopback_name = "Loopback Pseudo-Interface 1" # Common Windows loopback
    elif platform.system() == 'Darwin': # macOS
        loopback_name = "lo0" # macOS default

    try:
        specific_net_stats = metrics.get_net_stats_by_interface(loopback_name) # Renamed from get_net_stats(name)
        print(f"  Stats for {loopback_name}: {specific_net_stats}")
        print(f"  Bytes Received: {specific_net_stats.bytes_received}")
        print(f"  Bytes Sent: {specific_net_stats.bytes_sent}")
    except RuntimeError as e:
        print(f"  Could not get network stats for '{loopback_name}': {e}")
    except Exception as e:
        print(f"  An unexpected error occurred getting network stats: {e}")


    # Test calculate_network_throughput
    prev_net_snap = metrics.NetStats(
        interface_name="eth0", bytes_received=1000, bytes_sent=500, packets_received=10, packets_sent=5,
        errors_in=0, errors_out=0, drops_in=0, drops_out=0 # Corrected tx_errors to errors_out
    )
    curr_net_snap = metrics.NetStats(
        interface_name="eth0", bytes_received=2000, bytes_sent=1000, packets_received=20, packets_sent=10,
        errors_in=0, errors_out=0, drops_in=0, drops_out=0 # Corrected tx_errors to errors_out
    )
    net_time_delta_ms = 1000 # 1 second

    throughput = metrics.calculate_network_throughput(
        curr_net_snap, prev_net_snap, net_time_delta_ms
    )
    print(f"\nNetwork throughput (eth0) over {net_time_delta_ms}ms: Rx {throughput.rx_kbps:.2f} KB/s, Tx {throughput.tx_kbps:.2f} KB/s")

    # Test error for non-positive time_delta_ms
    try:
        metrics.calculate_network_throughput(curr_net_snap, prev_net_snap, 0)
    except ValueError as e:
        print(f"Caught expected error for 0 time_delta: {e}")

except AttributeError as e:
    print(f"Error accessing NetworkStats attributes/methods: {e}. Check NetworkStats binding in C++.")
except TypeError as e:
    print(f"Error creating NetworkStats object: {e}. Check NetworkStats constructor binding in C++.")
except Exception as e:
    print(f"An error occurred during network stats testing: {e}")

print("\n--- Testing DiskStats and DiskStatsReader ---")
def format_bytes(byte_count):
    """Helper to format bytes into KB, MB, GB, etc."""
    if byte_count is None:
        return "N/A"
    power = 1024
    n = 0
    power_labels = {0: '', 1: 'K', 2: 'M', 3: 'G', 4: 'T'}
    while byte_count >= power and n < len(power_labels):
        byte_count /= power
        n += 1
    return f"{byte_count:.2f} {power_labels[n]}B"

try:
    # DiskStats constructor: (device, read_bytes, write_bytes, read_time_ms, write_time_ms)
    disk_stats1 = metrics.DiskStats(
        device="sda", read_bytes=50000, write_bytes=10000, read_time_ms=500, write_time_ms=100
    )
    print(f"Initial disk stats: {disk_stats1}")
    print(f"Device: {disk_stats1.device}, Read Bytes: {disk_stats1.read_bytes}, Write Bytes: {disk_stats1.write_bytes}")

    # Test metrics.get_disk_stats_aggregated() (no args - aggregated)
    all_disk_stats_aggregated = metrics.get_disk_stats_aggregated() # Renamed from get_disk_stats()
    print("\nCurrent aggregated disk stats (single object):")
    if isinstance(all_disk_stats_aggregated, metrics.DiskStats):
        print(f"  Aggregated: {all_disk_stats_aggregated}")
        print(f"  Aggregated Read Bytes: {format_bytes(all_disk_stats_aggregated.read_bytes)}")
        print(f"  Aggregated Write Bytes: {format_bytes(all_disk_stats_aggregated.write_bytes)}")
    else:
        print(f"  Expected a single DiskStats object, but got: {type(all_disk_stats_aggregated)}")


    # Test metrics.get_disk_stats_by_device(device_name) (specific device)
    example_device_name = "nvme0n1" # !!! IMPORTANT: CHANGE THIS TO A REAL DEVICE ON YOUR SYSTEM FOR LIVE TESTING !!!
    if platform.system() == 'Windows':
        example_device_name = "PhysicalDrive0" # Common Windows physical drive
    print(f"\nAttempting to get stats for specific device: {example_device_name}")
    try:
        specific_device_stats = metrics.get_disk_stats_by_device(example_device_name) # Renamed from get_disk_stats(name)
        print(f"  Stats for '{example_device_name}': {specific_device_stats}")
        print(f"  Total bytes read for '{example_device_name}': {format_bytes(specific_device_stats.read_bytes)}")
        print(f"  Total bytes written for '{example_device_name}': {format_bytes(specific_device_stats.write_bytes)}")
    except RuntimeError as e:
        print(f"  Could not get stats for '{example_device_name}': {e}")
        print(f"  (Hint: Please check if '{example_device_name}' is a correct device name on your system using 'lsblk' on Linux, or device manager on Windows.)")
    except Exception as e:
        print(f"  An unexpected error occurred getting disk stats: {e}")


    # Test calculate_disk_io_throughput
    prev_disk_snap = metrics.DiskStats(
        device="nvme0n1", read_bytes=0, write_bytes=0, read_time_ms=0, write_time_ms=0
    )
    curr_disk_snap = metrics.DiskStats(
        device="nvme0n1", read_bytes=10240, write_bytes=5120, read_time_ms=100, write_time_ms=50
    )
    disk_time_delta_ms = 1000 # 1 second interval

    io_throughput = metrics.calculate_disk_io_throughput(
        curr_disk_snap, prev_disk_snap, disk_time_delta_ms
    )
    print(f"\nDisk I/O throughput (nvme0n1) over {disk_time_delta_ms}ms: Read {io_throughput.read_kbps:.2f} KB/s, Write {io_throughput.write_kbps:.2f} KB/s")

    # Test error for non-positive time_delta_ms
    try:
        metrics.calculate_disk_io_throughput(curr_disk_snap, prev_disk_snap, 0)
    except ValueError as e:
        print(f"Caught expected error for 0 time_delta: {e}")

except AttributeError as e:
    print(f"Error accessing DiskStats attributes/methods: {e}. Check DiskStats binding in C++.")
except TypeError as e:
    print(f"Error creating DiskStats object: {e}. Check DiskStats constructor binding in C++.")
except Exception as e:
    print(f"An error occurred during disk stats testing: {e}")

print("\n--- Testing Disk and DiskStatsReader (Live Monitoring Example) ---")

# --- Example 1: Get total aggregated stats ---
print("## Monitoring Total Disk I/O ##\n")

try:
    # Get initial stats
    last_stats_aggregated = metrics.get_disk_stats_aggregated() # Renamed
    last_time = time.time()

    print(f"Initial aggregated state: {last_stats_aggregated}")

    print("\nMonitoring I/O for 5 seconds...")
    time.sleep(1)

    # Get new stats
    current_stats_aggregated = metrics.get_disk_stats_aggregated() # Renamed
    current_time = time.time()

    # Calculate the difference in time and bytes
    time_delta = current_time - last_time
    read_delta = current_stats_aggregated.read_bytes - last_stats_aggregated.read_bytes
    write_delta = current_stats_aggregated.write_bytes - last_stats_aggregated.write_bytes

    # Calculate rate in bytes/sec
    read_rate = read_delta / time_delta
    write_rate = write_delta / time_delta

    print(f"Final aggregated state:   {current_stats_aggregated}")
    print(f"Read Rate:     {format_bytes(read_rate)}/s")
    print(f"Write Rate:    {format_bytes(write_rate)}/s")

except Exception as e:
    print(f"An error occurred during live disk monitoring example: {e}")

print("\n" + "="*40 + "\n")