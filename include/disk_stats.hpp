namespace SysytemDiskStats {

struct DiskStats {
    unsigned long long read_bytes, write_bytes;
    double read_rate, write_rate; // bytes/sec
};
}
