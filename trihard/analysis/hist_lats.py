import pandas as pd

import matplotlib.pyplot as plt

import os

# Get first parameter
if len(os.sys.argv) > 1:
    file_name = os.sys.argv[1]

# Load the CSV file
file_path = f"out/{file_name}.csv" # "out/lats.csv" #
latencies = pd.read_csv(file_path, header=None)

TSC_FREQ = 2.899999e9

latencies = latencies.astype(float)
latencies = latencies/TSC_FREQ

print(latencies.describe())

latencies = latencies.where(latencies > 3e-4).dropna()

# Plot the histogram
plt.figure(figsize=(10, 6))
plt.hist(latencies, bins=10000, color='blue', alpha=0.7, edgecolor='black', cumulative=True, density=True, facecolor='none')
plt.xscale('log')
plt.yscale('log')
# plt.xlim(1e-9, 10)
plt.xlabel('Latency (s)')
plt.ylabel('Frequency')
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.savefig(f"out/{file_name}.png", dpi=300, bbox_inches='tight')
