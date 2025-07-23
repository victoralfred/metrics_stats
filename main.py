import sysstats

cpu = sysstats.get_cpu_stats()
print(cpu.user, cpu.system, cpu.idle)

mem = sysstats.get_mem_stats()
print(mem.total, mem.available)