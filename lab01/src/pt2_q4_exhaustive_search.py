import subprocess
import re
import itertools
import pandas as pd
import time
from paretoset import paretoset

def run_simulation(l1i, l1d, l2, uf):
    exe_path = f"/gem5/tables_UF/tables_uf{uf}.exe"
    command = [
        "build/X86/gem5.opt",
        "configs/learning_gem5/part1/two_level.py",
        exe_path,
        f"--l1i_size={l1i}kB",
        f"--l1d_size={l1d}kB",
        f"--l2_size={l2}kB"
    ]
    
    subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    try:
        with open("m5out/stats.txt", "r") as stats_file:
            for line in stats_file:
                if "system.cpu.numCycles" in line:
                    return int(re.search(r'\d+', line).group())
    except FileNotFoundError:
        return None
    return None

l1d_sizes = [2, 4, 8, 16, 32, 64]
l1i_sizes = [2, 4, 8, 16, 32, 64]
l2_sizes = [128, 256, 512, 1024]
unrolling_factors = [2, 4, 8, 16, 32]

results = []
combinations = list(itertools.product(l1i_sizes, l1d_sizes, l2_sizes, unrolling_factors))
total_runs = len(combinations)

print(f"Starting exhaustive search over {total_runs} combinations...")
start_time = time.time()

for i, (l1i, l1d, l2, uf) in enumerate(combinations, 1):
    total_memory_kb = l1i + l1d + l2
    cycles = run_simulation(l1i, l1d, l2, uf)
    
    if cycles is not None:
        results.append({
            "L1I_kB": l1i,
            "L1D_kB": l1d,
            "L2_kB": l2,
            "UnrollingFactor": uf,
            "totalMemoryKB": total_memory_kb,
            "latencyCC": cycles
        })
    print(f"Completed {i}/{total_runs}", end='\r')

end_time = time.time()
total_time = end_time - start_time
hours, rem = divmod(total_time, 3600)
minutes, seconds = divmod(rem, 60)
time_str = f"{int(hours)}h {int(minutes)}m {seconds:.2f}s"

# Save all results
df = pd.DataFrame(results)
df.to_csv("exhaustive_search_results.csv", index=False)
print(f"\nExhaustive search complete in {time_str}. Results saved to exhaustive_search_results.csv")

# Save total time to a text file
with open("execution_time.txt", "w") as f:
    f.write(f"Total exhaustive search time: {time_str}\n")

# Extract and save Pareto Frontier
mask = paretoset(df[["totalMemoryKB", "latencyCC"]], sense=["min", "min"])
pareto_front = df[mask]
pareto_front.to_csv("pareto_frontier.csv", index=False)
print(f"Pareto Frontier identified with {len(pareto_front)} configurations. Saved to pareto_frontier.csv")
