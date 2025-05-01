#!/usr/bin/env python3
import os
import re
import pandas as pd

RESULTS_DIR = 'results'

# Regex to strip ANSI color codes from log lines
# This is used to clean up the log lines for easier parsing
ansi_escape = re.compile(r'\x1b\[[0-9;]*m')

# timestamps are in the form of  [1234.567890123]
timestamp_pattern = re.compile(r'\[([0-9]+\.[0-9]+)\]')

records = []

for root, dirs, files in os.walk(RESULTS_DIR):
    if 'stdout.log' not in files:
        continue

    # Parse scheduler and invocation count from path: e.g. "random_100"
    rel = os.path.relpath(root, RESULTS_DIR)
    try:
        scheduler, host_str = rel.split('_')
        host = int(host_str)
    except ValueError:
        continue

    max_time = 0.0
    with open(os.path.join(root, 'stdout.log'), 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            clean = ansi_escape.sub('', line)
            for tstr in timestamp_pattern.findall(clean):
                t = float(tstr)
                if t > max_time:
                    max_time = t
    # log the max time
    if max_time > 0:
        records.append({
            'scheduler': scheduler,
            'hosts': host,
            'sim_time_s': max_time
        })

# Build DataFrame
df = pd.DataFrame.from_records(records)
df = df.sort_values(['scheduler','hosts'])
# Print to console
print("\nAggregated Simulation End Times:\n")
print(df.to_string(index=False))

# Save to CSV for later use
output_csv = 'aggregated_results.csv'
df.to_csv(output_csv, index=False)
print(f"\nSaved aggregated results to {output_csv}")