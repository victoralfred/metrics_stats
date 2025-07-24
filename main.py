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
