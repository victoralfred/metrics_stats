import sys
import os

# --- Configuration for your vcpkg installation ---
# Option 1: Get VCPKG_ROOT from environment variable (recommended for flexibility)
vcpkg_root = os.environ.get("VCPKG_ROOT")

# Option 2: Hardcode the path to your vcpkg root (less flexible, but works if env var isn't set)
# vcpkg_root = "/path/to/your/vcpkg" # <--- UNCOMMENT AND REPLACE IF YOU DON'T USE ENV VAR

if vcpkg_root is None:
    print("Error: VCPKG_ROOT environment variable not set.")
    print("Please set it to the root directory of your vcpkg installation,")
    print("or uncomment and set 'vcpkg_root' directly in this script.")
    sys.exit(1) # Exit if vcpkg_root is not defined

# Construct the full path to the directory containing your .so file
# Based on your provided structure: vcpkg_root/installed/x64-linux/lib
module_lib_path = os.path.join(vcpkg_root, "installed", "x64-linux", "lib")

# Add this directory to sys.path
if os.path.isdir(module_lib_path):
    sys.path.append(module_lib_path)
    print(f"Added '{module_lib_path}' to sys.path for module discovery.")
else:
    print(f"Warning: Directory '{module_lib_path}' does not exist.")
    print("Please ensure vcpkg is installed correctly and sysstats:x64-linux was built successfully.")
    print("Current working directory:", os.getcwd())
    print("Contents of vcpkg_root/installed:", os.listdir(os.path.join(vcpkg_root, "installed")))
    # You might want to exit here if the module is critical
    # sys.exit(1)

# --- Now, attempt to import the sysstats module ---
try:
    import sysstats
    print("Successfully imported 'sysstats' module!")

    # --- You can now use the functions/classes from the sysstats module ---
    # For example, if sysstats has a function called `get_cpu_info()`:
    # cpu_info = sysstats.get_cpu_info()
    # print(f"CPU Information: {cpu_info}")

    # Or if it has a function to get memory usage:
    # mem_usage = sysstats.get_memory_usage()
    # print(f"Memory Usage: {mem_usage}")

    # Replace these examples with actual functions provided by your sysstats module
    # To see what's available, you could use:
    # print(dir(sysstats))

except ImportError as e:
    print(f"Error: Could not import 'sysstats' module.")
    print(f"Reason: {e}")
    print("Please check:")
    print(f"  1. If '{module_lib_path}' is indeed the correct path.")
    print(f"  2. If 'sysstats.cpython-39-x86_64-linux-gnu.so' exists in that directory.")
    print(f"  3. If your Python version (3.9 in this case) matches the compiled module.")
    print(f"  4. If all necessary shared libraries (dependencies of the .so) are available on your system.")
    print("Current sys.path:", sys.path)
except Exception as e:
    print(f"An unexpected error occurred after import: {e}")