import subprocess
import re
import matplotlib.pyplot as plt

def run_simulation(executable, l1d_size="8kB"):
    command = [
        "build/X86/gem5.opt",
        "configs/learning_gem5/part1/two_level.py",
        executable,
        "--l1i_size=8kB",
        f"--l1d_size={l1d_size}",
        "--l2_size=128kB"
    ]
    
    subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    try:
        with open("m5out/stats.txt", "r") as stats_file:
            for line in stats_file:
                if "system.cpu.numCycles" in line:
                    return int(re.search(r'\d+', line).group())
    except FileNotFoundError:
        return 0
    return 0

print("--- Unrolling Factors Exploration ---")
unrolling_factors = [2, 4, 8, 16, 32]
uf_cycles = []

for uf in unrolling_factors:
    exe_path = f"/gem5/tables_UF/tables_uf{uf}.exe"
    cycles = run_simulation(exe_path)
    uf_cycles.append(cycles)
    print(f"Unrolling Factor {uf}: {cycles} cycles")

print("\n--- L1 Data Cache Size Exploration ---")
l1d_sizes = ["2kB", "4kB", "8kB", "16kB", "32kB", "64kB"]
l1d_sizes_int = [int(re.search(r'\d+', size).group()) for size in l1d_sizes]
l1d_cycles = []
base_exe_path = "/gem5/tables_UF/tables.exe"

for size in l1d_sizes:
    cycles = run_simulation(base_exe_path, l1d_size=size)
    l1d_cycles.append(cycles)
    print(f"L1D Size {size}: {cycles} cycles")

# Generate Unrolling Factor Plot
plt.figure()
plt.plot(unrolling_factors, uf_cycles, marker='o', linestyle='-', color='b')
plt.title("Impact of Unrolling Factor on Execution Cycles")
plt.xlabel("Unrolling Factor")
plt.ylabel("Number of Cycles")
plt.xticks(unrolling_factors)
plt.grid(True)
plt.savefig("unrolling_factors_plot.png")
plt.close()

# Generate L1D Cache Size Plot
plt.figure()
plt.plot(l1d_sizes_int, l1d_cycles, marker='s', linestyle='-', color='r')
plt.title("Impact of L1D Cache Size on Execution Cycles")
plt.xlabel("L1D Cache Size (kB)")
plt.ylabel("Number of Cycles")
plt.xticks(l1d_sizes_int)
plt.grid(True)
plt.savefig("l1d_cache_size_plot.png")
plt.close()
