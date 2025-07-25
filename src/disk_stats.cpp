#include "disk_stats.hpp"

#include <stdexcept>
#include <vector>
#include <string>
#include <iostream> // For std::cerr in tests
#include "logger.hpp"
// Platform-specific headers
#if defined(_WIN32) || defined(__WIN64__)
    #define _WIN32_DCOM
    #include <comdef.h>
    #include <Wbemidl.h>
    #pragma comment(lib, "wbemuuid.lib")
#elif defined(__APPLE__)
    #include <CoreFoundation/CoreFoundation.h>
    #include <IOKit/IOKitLib.h> // Corrected header
    #include <IOKit/storage/IOBlockStorageDriver.h>
    #include <IOKit/storage/IOMedia.h>
#elif defined(__linux__)
    #include <fstream>
    #include <istream>
    #include <sstream>
    #include <string_view>
    #include <cctype> // For ::isdigit
#endif

namespace SystemDiskStats {

// --- Forward Declarations for Private Platform-Specific Helpers ---
// Note: These helpers return a vector of stats, which is necessary for the public API
// to perform aggregation or selection. This is a practical interpretation of the header's intent.
static std::vector<DiskStats> getRawWindowsDiskStats();
static std::vector<DiskStats> getRawLinuxDiskStats();
static std::vector<DiskStats> getRawMacDiskStats();
static std::vector<DiskStats> getRawUnsupportedDiskStats();

/// @brief Private helper to dispatch to the correct platform-specific function.
/// This avoids duplicating the #ifdef logic in every public method.
static std::vector<DiskStats> getAllStats() {
#if defined(_WIN32) || defined(__WIN64__)
    return getRawWindowsDiskStats();
#elif defined(__linux__)
    return getRawLinuxDiskStats();
#elif defined(__APPLE__)
    return getRawMacDiskStats();
#else
    return getRawUnsupportedDiskStats();
#endif
}


// --- Public API Implementation ---

DiskStats DiskStatsReader::getDiskStats() {
    const auto all_stats = getAllStats();
    
    DiskStats aggregated_stats{"aggregated", 0, 0, 0, 0};
    for (const auto& stats : all_stats) {
        aggregated_stats.read_bytes += stats.read_bytes;
        aggregated_stats.write_bytes += stats.write_bytes;
        aggregated_stats.read_time_ms += stats.read_time_ms;
        aggregated_stats.write_time_ms += stats.write_time_ms;
    }
    
    return aggregated_stats;
}

DiskStats DiskStatsReader::getDiskStats(const std::string& device_name) {
    const auto all_stats = getAllStats();
    
    for (const auto& stats : all_stats) {
        if (stats.device == device_name) {
            return stats;
        }
    }
    
    throw std::runtime_error("Device '" + device_name + "' not found or has no stats.");
}

#if defined(__linux__)
// Public parser implementation for Linux
DiskStats DiskStatsReader::parseDiskStatLine(const std::string& line) {
    std::istringstream iss(line);
    unsigned long long major, minor, sectors_read, ms_read, sectors_written, ms_written, dummy;
    std::string device_name;

    // Attempt to read all 14 fields
    iss >> major >> minor >> device_name // 3 fields
        >> dummy >> dummy >> sectors_read >> ms_read // +4 fields = 7
        >> dummy >> dummy >> sectors_written >> ms_written // +4 fields = 11
        >> dummy >> dummy >> dummy; // +3 fields = 14

    // Check if any parsing error occurred. iss.fail() is true if an extraction operation failed.
    // This correctly handles cases like too few fields or non-numeric data where numbers are expected.
    if (iss.fail()) { // <-- Corrected condition: removed '&& !iss.eof()'
        throw std::runtime_error("Failed to parse diskstat line: " + line + " (Not enough fields or invalid data)");
    }
    // Assuming 512 bytes per sector as is standard for /proc/diskstats
    return DiskStats{ std::move(device_name), sectors_read * 512, sectors_written * 512, ms_read, ms_written };
}
#else
// On non-Linux platforms, this function is not applicable.
DiskStats DiskStatsReader::parseDiskStatLine(const std::string& line) {
    (void)line; // Avoid unused parameter warning
    throw std::logic_error("parseDiskStatLine is only available on Linux.");
}
#endif


// --- Private Platform-Specific Implementations ---

#if defined(_WIN32) || defined(__WIN64__)
class ComInitializer {
public:
    ComInitializer() {
        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) { // RPC_E_CHANGED_MODE means COM was already initialized differently, which is often okay.
            throw std::runtime_error("Failed to initialize COM library.");
        }
        if (FAILED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL))) {
            CoUninitialize();
            throw std::runtime_error("Failed to initialize COM security.");
        }
    }
    ~ComInitializer() { CoUninitialize(); }
};

static std::vector<DiskStats> getRawWindowsDiskStats() {
    ComInitializer com_init;
    IWbemLocator* pLoc = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr)) {
        std::cerr << "CoCreateInstance failed: " << std::hex << hr << std::endl;
        throw std::runtime_error("Failed to create IWbemLocator.");
    }
    IWbemServices* pSvc = nullptr;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr)) { 
        pLoc->Release(); 
        std::cerr << "ConnectServer failed: " << std::hex << hr << std::endl;
        throw std::runtime_error("Could not connect to WMI."); 
    }
    pLoc->Release();
    
    // Set security levels on the proxy
    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr)) {
        pSvc->Release();
        std::cerr << "CoSetProxyBlanket failed: " << std::hex << hr << std::endl;
        throw std::runtime_error("Could not set WMI security blanket.");
    }

    IEnumWbemClassObject* pEnumerator = nullptr;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT Name, DiskReadBytesPersec, DiskWriteBytesPersec, PercentDiskReadTime, PercentDiskWriteTime FROM Win32_PerfRawData_PerfDisk_PhysicalDisk"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr)) { 
        pSvc->Release(); 
        std::cerr << "WMI query failed: " << std::hex << hr << std::endl;
        throw std::runtime_error("WMI query failed."); 
    }
    
    std::vector<DiskStats> all_stats;
    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    while (pEnumerator && pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn != 0) {
        VARIANT vtProp;
        VariantInit(&vtProp); // Initialize variant

        pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        std::wstring wname(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
        VariantClear(&vtProp);
        
        if (wname.find(L"_Total") != std::wstring::npos) { // Skip aggregated stats
            pclsObj->Release(); 
            continue; 
        }
        
        std::string device_name;
        // Calculate required buffer size for MultiByteToWideChar
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), (int)wname.length(), NULL, 0, NULL, NULL);
        if (size_needed > 0) {
            device_name.resize(size_needed);
            WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), (int)wname.length(), &device_name[0], size_needed, NULL, NULL);
        } else {
            device_name = "UnknownDevice"; // Fallback for conversion error
        }

        unsigned long long read_bytes = 0, write_bytes = 0, read_time_ms = 0, write_time_ms = 0;
        
        VariantInit(&vtProp);
        pclsObj->Get(L"DiskReadBytesPersec", 0, &vtProp, 0, 0); 
        if (vtProp.vt == VT_BSTR) read_bytes = _wtoi64(vtProp.bstrVal); 
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        pclsObj->Get(L"DiskWriteBytesPersec", 0, &vtProp, 0, 0); 
        if (vtProp.vt == VT_BSTR) write_bytes = _wtoi64(vtProp.bstrVal); 
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        pclsObj->Get(L"PercentDiskReadTime", 0, &vtProp, 0, 0); 
        // PercentDiskReadTime is in 100-nanosecond units. Convert to ms.
        if (vtProp.vt == VT_BSTR) read_time_ms = _wtoi64(vtProp.bstrVal) / 10000; 
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        pclsObj->Get(L"PercentDiskWriteTime", 0, &vtProp, 0, 0); 
        // PercentDiskWriteTime is in 100-nanosecond units. Convert to ms.
        if (vtProp.vt == VT_BSTR) write_time_ms = _wtoi64(vtProp.bstrVal) / 10000; 
        VariantClear(&vtProp);
        
        // Example: "0 C:" -> "PhysicalDrive0"
        // Find the first space to extract the drive number if present
        size_t first_space = device_name.find(' ');
        if (first_space != std::string::npos && device_name.rfind("PhysicalDrive", 0) == std::string::npos) {
            // Only prepend "PhysicalDrive" if it's not already there and we found a space
            device_name = "PhysicalDrive" + device_name.substr(0, first_space);
        } else if (device_name == "_Total") {
             // This case should be skipped by the earlier check, but as a safeguard
             pclsObj->Release(); continue;
        }

        all_stats.emplace_back(std::move(device_name), read_bytes, write_bytes, read_time_ms, write_time_ms);
        pclsObj->Release();
    }
    if(pSvc) pSvc->Release();
    if(pEnumerator) pEnumerator->Release();
    return all_stats;
}

#elif defined(__APPLE__)
// Helper struct for proper IOObjectRelease
template <typename T> struct IOObjectRef {
    T object;
    explicit IOObjectRef(T obj = IO_OBJECT_NULL) : object(obj) {} // Initialize with IO_OBJECT_NULL
    IOObjectRef(const IOObjectRef&) = delete; // Delete copy constructor
    IOObjectRef& operator=(const IOObjectRef&) = delete; // Delete copy assignment
    IOObjectRef(IOObjectRef&& other) noexcept : object(other.object) { other.object = IO_OBJECT_NULL; } // Move constructor
    IOObjectRef& operator=(IOObjectRef&& other) noexcept { 
        if (this != &other) {
            if (object) IOObjectRelease(object);
            object = other.object;
            other.object = IO_OBJECT_NULL;
        }
        return *this;
    }
    ~IOObjectRef() { if (object) IOObjectRelease(object); }
    T get() const { return object; }
    T* operator&() { return &object; } // Allows direct passing to functions like IOIteratorNext
};

// Helper struct for proper CFRelease
template <typename T> struct CFRef {
    T ref;
    explicit CFRef(T r = nullptr) : ref(r) {}
    CFRef(const CFRef&) = delete;
    CFRef& operator=(const CFRef&) = delete;
    CFRef(CFRef&& other) noexcept : ref(other.ref) { other.ref = nullptr; }
    CFRef& operator=(CFRef&& other) noexcept {
        if (this != &other) {
            if (ref) CFRelease(ref);
            ref = other.ref;
            other.ref = nullptr;
        }
        return *this;
    }
    ~CFRef() { if (ref) CFRelease(ref); }
    T get() const { return ref; }
};

static std::vector<DiskStats> getRawMacDiskStats() {
    IOObjectRef<io_iterator_t> iter;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(kIOBlockStorageDriverClass), &iter.object) != kIOReturnSuccess) {
        throw std::runtime_error("IOServiceGetMatchingServices failed for IOBlockStorageDriverClass.");
    }
    std::vector<DiskStats> all_stats;
    io_registry_entry_t service;
    while ((service = IOIteratorNext(iter.get()))) {
        IOObjectRef<io_registry_entry_t> service_ref(service); // Auto-releases service

        // Get the parent entry (IOMedia object)
        io_registry_entry_t parent_entry = IO_OBJECT_NULL;
        if (IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent_entry) != kIOReturnSuccess) {
            continue; // Skip if no parent
        }
        IOObjectRef<io_registry_entry_t> parent_ref(parent_entry); // Auto-releases parent_entry

        // Check if it's a leaf node (partition) and skip it
        CFRef<CFBooleanRef> is_leaf_ref((CFBooleanRef)IORegistryEntryCreateCFProperty(parent_ref.get(), CFSTR(kIOMediaLeafKey), kCFAllocatorDefault, 0));
        if (is_leaf_ref.get() && CFBooleanGetValue(is_leaf_ref.get()) == true) {
            continue;
        }
        
        // Get the BSD name (e.g., "disk0")
        CFRef<CFStringRef> bsdNameRef((CFStringRef)IORegistryEntryCreateCFProperty(parent_ref.get(), CFSTR(kIOMediaBSDNameKey), kCFAllocatorDefault, 0));
        if (!bsdNameRef.get()) {
            continue;
        }
        char bsd_name[64];
        if (!CFStringGetCString(bsdNameRef.get(), bsd_name, sizeof(bsd_name), kCFStringEncodingUTF8)) {
            continue; // Skip if cannot get BSD name
        }

        // Get disk statistics dictionary
        CFRef<CFDictionaryRef> statsDict((CFDictionaryRef)IORegistryEntryCreateCFProperty(service_ref.get(), CFSTR(kIOBlockStorageDriverStatisticsKey), kCFAllocatorDefault, 0));
        if (!statsDict.get()) {
            continue;
        }

        unsigned long long rd_b = 0, wr_b = 0, rd_t = 0, wr_t = 0;
        CFNumberRef num;

        if ((num = (CFNumberRef)CFDictionaryGetValue(statsDict.get(), CFSTR(kIOBlockStorageDriverStatisticsBytesReadKey)))) {
            CFNumberGetValue(num, kCFNumberSInt64Type, &rd_b);
        }
        if ((num = (CFNumberRef)CFDictionaryGetValue(statsDict.get(), CFSTR(kIOBlockStorageDriverStatisticsBytesWrittenKey)))) {
            CFNumberGetValue(num, kCFNumberSInt64Type, &wr_b);
        }
        // Times are in nanoseconds, convert to milliseconds
        if ((num = (CFNumberRef)CFDictionaryGetValue(statsDict.get(), CFSTR(kIOBlockStorageDriverStatisticsTotalReadTimeKey)))) {
            CFNumberGetValue(num, kCFNumberSInt64Type, &rd_t);
            rd_t /= 1000000; // Nanoseconds to milliseconds
        }
        if ((num = (CFNumberRef)CFDictionaryGetValue(statsDict.get(), CFSTR(kIOBlockStorageDriverStatisticsTotalWriteTimeKey)))) {
            CFNumberGetValue(num, kCFNumberSInt64Type, &wr_t);
            wr_t /= 1000000; // Nanoseconds to milliseconds
        }
        
        all_stats.emplace_back(bsd_name, rd_b, wr_b, rd_t, wr_t);
    }
    return all_stats;
}

#elif defined(__linux__)
namespace { // Anonymous namespace for Linux-only internal helpers
// Helper function to determine if a device name represents a physical drive
// Skips loopback devices, ramdisks, and device mapper partitions (e.g., dm-0)
// Also skips partitions (e.g., sda1, sdb2) by checking if the last char is a digit
bool isPhysicalDrive(std::string_view device_name) {
    // Debugging: Show which device name is being checked
    SystemMetricsLogger::Logger::debug("isPhysicalDrive: Checking '" + std::string(device_name) + "'");

    // Exclude common non-physical devices by prefix
    if (device_name.rfind("loop", 0) == 0 || device_name.rfind("ram", 0) == 0 || device_name.rfind("dm-", 0) == 0 || device_name.rfind("zd", 0) == 0) {
        SystemMetricsLogger::Logger::debug("isPhysicalDrive: Excluded by loop/ram/dm-/zd prefix.");
        return false;
    }

    // Special handling for NVMe devices (e.g., nvme0n1, nvme0n1p1)
    if (device_name.rfind("nvme", 0) == 0) {
        // A physical NVMe drive name typically looks like nvmeXnY (e.g., nvme0n1)
        // A partition on an NVMe drive looks like nvmeXnYpZ (e.g., nvme0n1p1)
        size_t p_pos = device_name.rfind('p');
        if (p_pos != std::string_view::npos && p_pos > 0 && p_pos + 1 < device_name.length() && ::isdigit(device_name[p_pos + 1])) {
            // Check if the 'p' is followed by a digit (like nvme0n1p1)
            SystemMetricsLogger::Logger::debug("isPhysicalDrive: Excluded as NVMe partition ('p' followed by digit).");
            return false;
        }
        // If it's an NVMe device and not identified as a partition by the above rule, include it.
        SystemMetricsLogger::Logger::debug("isPhysicalDrive: Included as NVMe drive.");
        return true;
    }

    // For non-NVMe devices (like sda, hda, vda), exclude if they end in a digit (likely a partition like sda1)
    if (!device_name.empty() && ::isdigit(device_name.back())) {
        // We've already handled NVMe devices, so this applies to others like 'sda1'
        SystemMetricsLogger::Logger::debug("isPhysicalDrive: Excluded because it ends with a digit (likely a non-NVMe partition).");
        return false;
    }

    // Default: include if it doesn't match any exclusion rules. (e.g., sda, hda)
    SystemMetricsLogger::Logger::debug("isPhysicalDrive: Included (default case).");
    return true;
}
}
static std::vector<DiskStats> getRawLinuxDiskStats() {
    std::ifstream file("/proc/diskstats");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open /proc/diskstats. Ensure you have permissions (e.g., run as root or with sudo).");
    }
    
    std::vector<DiskStats> all_stats;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try {
            /// @todo: Remove Add debug output to help with parsing issues
            DiskStats stats = DiskStatsReader::parseDiskStatLine(line);
            if (isPhysicalDrive(stats.device)) {
                all_stats.push_back(std::move(stats));
            }
        } catch (const std::runtime_error& e) { 
            // Ignore malformed lines, but log to stderr for debugging if needed
            // std::cerr << "Warning: Ignoring malformed diskstat line: " << line << " Error: " << e.what() << std::endl;
        } catch (const std::logic_error&) {
            // This should not happen on Linux as parseDiskStatLine is specialized.
            // If it does, something is wrong with the build environment.
            std::cerr << "Fatal Error: logic_error from parseDiskStatLine on Linux!" << std::endl;
        }
    }
    return all_stats;
}

#else 
static std::vector<DiskStats> getRawUnsupportedDiskStats() {
    throw std::runtime_error("Disk statistics are not supported on this platform.");
}
#endif

} // namespace SystemDiskStats
