#!/usr/bin/env python3
"""
Universal Load Test Results Plotter
Handles both read and write workload log formats.

Usage:
    python plot_results_universal.py <log_pattern> [--output-dir DIR]

Example:
    python plot_results_universal.py "*_*thread.txt" --output-dir graphs/
"""

import os
import re
import sys
import glob
import argparse
import matplotlib.pyplot as plt

def parse_log_file(filepath):
    """Parse load test log file - handles both read and write formats."""
    data = {}

    try:
        with open(filepath, 'r') as f:
            content = f.read()

        filename = os.path.basename(filepath)

        # Extract thread count
        thread_match = re.search(r'_(\d+)thread', filename)
        if thread_match:
            data['threads'] = int(thread_match.group(1))
        else:
            thread_match = re.search(r'Threads:\s*(\d+)', content)
            data['threads'] = int(thread_match.group(1)) if thread_match else 0

        # Extract workload type
        filename_lower = filename.lower()
        if 'write_' in filename_lower or '_write' in filename_lower or filename_lower.startswith('write'):
            data['workload'] = 'Write'
        elif 'read_' in filename_lower or '_read' in filename_lower or filename_lower.startswith('read'):
            data['workload'] = 'Read'
        elif 'mixed' in filename_lower:
            data['workload'] = 'Mixed'
        else:
            data['workload'] = 'Unknown'

        # Extract throughput - handle both formats
        # Write format: "Throughput:\n  Total: 258.75 req/s"
        # Read format: "Throughput: 838.04 req/s"
        throughput_match = re.search(r'Total:\s*([\d.]+)\s*req/s', content)
        if throughput_match:
            data['throughput'] = float(throughput_match.group(1))
        else:
            throughput_match = re.search(r'Throughput:\s*([\d.]+)\s*req/s', content)
            data['throughput'] = float(throughput_match.group(1)) if throughput_match else 0

        # Extract effective throughput (write workloads)
        effective_match = re.search(r'Effective:\s*([\d.]+)\s*req/s', content)
        if effective_match:
            data['effective_throughput'] = float(effective_match.group(1))

        # Extract latency statistics
        mean_match = re.search(r'Mean:\s*([\d.]+)', content)
        data['mean_latency'] = float(mean_match.group(1)) if mean_match else 0

        median_match = re.search(r'Median:\s*([\d.]+)', content)
        data['median_latency'] = float(median_match.group(1)) if median_match else 0

        p95_match = re.search(r'P95:\s*([\d.]+)', content)
        data['p95_latency'] = float(p95_match.group(1)) if p95_match else 0

        p99_match = re.search(r'P99:\s*([\d.]+)', content)
        data['p99_latency'] = float(p99_match.group(1)) if p99_match else 0

        min_match = re.search(r'Min:\s*([\d.]+)', content)
        data['min_latency'] = float(min_match.group(1)) if min_match else 0

        max_match = re.search(r'Max:\s*([\d.]+)', content)
        data['max_latency'] = float(max_match.group(1)) if max_match else 0

        return data

    except Exception as e:
        print(f"  ✗ Error parsing {filepath}: {e}")
        return None

def plot_throughput(data_by_workload, output_dir):
    """Plot throughput vs threads."""
    plt.figure(figsize=(10, 6))

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list]
        throughput = [d['throughput'] for d in data_list]
        plt.plot(threads, throughput, marker='o', linewidth=2.5, markersize=8, 
                label=f'{workload} Workload')

    plt.xlabel('Number of Clients', fontsize=13, fontweight='bold')
    plt.ylabel('Throughput (requests/sec)', fontsize=13, fontweight='bold')
    plt.title('Throughput vs Number of Clients', fontsize=15, fontweight='bold', pad=20)
    plt.xlim(left=0)
    plt.ylim(bottom=0)
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.tight_layout()

    output_path = os.path.join(output_dir, 'throughput_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def plot_latency(data_by_workload, output_dir, metric='mean'):
    """Plot latency vs threads."""
    plt.figure(figsize=(10, 6))

    metric_key = f'{metric}_latency'
    metric_label = metric.upper() if metric in ['p95', 'p99'] else metric.capitalize()

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if metric_key in d and d[metric_key] > 0]
        latency = [d[metric_key] for d in data_list if metric_key in d and d[metric_key] > 0]

        if threads:
            marker = 'o' if metric == 'mean' else ('s' if metric == 'p95' else '^')
            plt.plot(threads, latency, marker=marker, linewidth=2.5, markersize=8,
                    label=f'{workload} Workload')

    plt.xlabel('Number of Clients', fontsize=13, fontweight='bold')
    plt.ylabel(f'{metric_label} Latency (ms)', fontsize=13, fontweight='bold')
    plt.title(f'{metric_label} Latency vs Number of Clients', fontsize=15, fontweight='bold', pad=20)
    plt.xlim(left=0)
    plt.ylim(bottom=0)
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.tight_layout()

    output_path = os.path.join(output_dir, f'{metric}_latency_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def print_summary_table(data_by_workload):
    """Print a summary table."""
    print("\n" + "="*70)
    print("LOAD TEST RESULTS SUMMARY")
    print("="*70)

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])

        print(f"\n{workload} Workload:")
        print("-" * 70)
        print(f"{'Load':<10} {'Throughput':<15} {'Mean Lat':<12} {'P95 Lat':<12}")
        print(f"{'':10} {'(req/s)':<15} {'(ms)':<12} {'(ms)':<12}")
        print("-" * 70)

        for d in data_list:
            threads = d.get('threads', 'N/A')
            throughput = f"{d.get('throughput', 0):.2f}"
            mean_lat = f"{d.get('mean_latency', 0):.2f}"
            p95_lat = f"{d.get('p95_latency', 0):.2f}"

            print(f"{threads:<10} {throughput:<15} {mean_lat:<12} {p95_lat:<12}")

def main():
    parser = argparse.ArgumentParser(description='Plot load test results (universal format)')
    parser.add_argument('pattern', help='File pattern (e.g., "*_*thread.txt")')
    parser.add_argument('--output-dir', default='graphs', help='Output directory')

    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    # Find files
    print("\n" + "="*70)
    print("UNIVERSAL LOAD TEST RESULTS PLOTTER")
    print("="*70)

    files = glob.glob(args.pattern)
    if not files:
        print(f"\nError: No files found matching: {args.pattern}")
        sys.exit(1)

    print(f"\nFound {len(files)} log files")

    # Parse all files
    all_data = []
    for filepath in files:
        print(f"  Parsing: {filepath}")
        data = parse_log_file(filepath)
        if data and 'threads' in data and 'throughput' in data:
            all_data.append(data)

    if not all_data:
        print("\nError: No valid data extracted")
        sys.exit(1)

    # Group by workload
    data_by_workload = {}
    for d in all_data:
        workload = d.get('workload', 'Unknown')
        if workload not in data_by_workload:
            data_by_workload[workload] = []
        data_by_workload[workload].append(d)

    print(f"\nProcessed {len(all_data)} results across {len(data_by_workload)} workload(s)")
    print(f"Workloads: {', '.join(data_by_workload.keys())}")

    # Generate plots
    print("\n" + "="*70)
    print("GENERATING GRAPHS")
    print("="*70)
    print()

    plot_throughput(data_by_workload, args.output_dir)
    plot_latency(data_by_workload, args.output_dir, 'mean')
    plot_latency(data_by_workload, args.output_dir, 'median')
    plot_latency(data_by_workload, args.output_dir, 'p95')
    plot_latency(data_by_workload, args.output_dir, 'p99')

    # Print summary
    print_summary_table(data_by_workload)

    print("\n" + "="*70)
    print("ALL GRAPHS GENERATED SUCCESSFULLY")
    print("="*70)
    print(f"\nGraphs saved to: {args.output_dir}/")
    print("="*70 + "\n")

if __name__ == '__main__':
    main()