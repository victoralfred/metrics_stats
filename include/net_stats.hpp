namespace SystemNetworkStats {
struct NetStats {
    unsigned long long rx_bytes, tx_bytes;
    double rx_rate, tx_rate; // bytes/sec
};
}