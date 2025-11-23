import os
import re
import sys
import glob
import argparse
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

def parse_log_file(filepath):
    """
    Parse a single log file and extract metrics.

    Returns:
        dict: {
            'threads': int,
            'duration': int,
            'total_requests': int,
            'successful': int,
            'failed': int,
            'throughput': float,
            'mean_latency': float,
            'median_latency': float,
            'p95_latency': float,
            'p99_latency': float,
            'min_latency': float,
            'max_latency': float,
            'workload': str
        }
    """
    data = {}

    try:
        with open(filepath, 'r') as f:
            content = f.read()

        # Extract filename to determine thread count and workload
        filename = os.path.basename(filepath)

        # Try to extract thread count from filename (e.g., "read_4thread.txt")
        thread_match = re.search(r'_(\d+)thread', filename)
        if thread_match:
            data['threads'] = int(thread_match.group(1))
        else:
            # Try to extract from log content
            thread_match = re.search(r'Threads:\s*(\d+)', content)
            if thread_match:
                data['threads'] = int(thread_match.group(1))
            else:
                data['threads'] = 0

        # Extract workload type from filename
        if 'read' in filename.lower():
            data['workload'] = 'read'
        elif 'write' in filename.lower():
            data['workload'] = 'write'
        elif 'mixed' in filename.lower():
            data['workload'] = 'mixed'
        else:
            data['workload'] = 'unknown'

        # Extract duration
        duration_match = re.search(r'Duration:\s*(\d+)\s*seconds', content)
        if duration_match:
            data['duration'] = int(duration_match.group(1))
        else:
            data['duration'] = 0

        # Extract request counts
        total_match = re.search(r'Total Requests:\s*(\d+)', content)
        if total_match:
            data['total_requests'] = int(total_match.group(1))

        success_match = re.search(r'Successful:\s*(\d+)', content)
        if success_match:
            data['successful'] = int(success_match.group(1))

        failed_match = re.search(r'Failed:\s*(\d+)', content)
        if failed_match:
            data['failed'] = int(failed_match.group(1))

        # Extract throughput
        throughput_match = re.search(r'Throughput:\s*([\d.]+)\s*req/s', content)
        if throughput_match:
            data['throughput'] = float(throughput_match.group(1))

        # Extract latency statistics
        mean_match = re.search(r'Mean:\s*([\d.]+)', content)
        if mean_match:
            data['mean_latency'] = float(mean_match.group(1))

        median_match = re.search(r'Median:\s*([\d.]+)', content)
        if median_match:
            data['median_latency'] = float(median_match.group(1))

        p95_match = re.search(r'P95:\s*([\d.]+)', content)
        if p95_match:
            data['p95_latency'] = float(p95_match.group(1))

        p99_match = re.search(r'P99:\s*([\d.]+)', content)
        if p99_match:
            data['p99_latency'] = float(p99_match.group(1))

        min_match = re.search(r'Min:\s*([\d.]+)', content)
        if min_match:
            data['min_latency'] = float(min_match.group(1))

        max_match = re.search(r'Max:\s*([\d.]+)', content)
        if max_match:
            data['max_latency'] = float(max_match.group(1))

        return data

    except Exception as e:
        print(f"Error parsing {filepath}: {e}")
        return None

def plot_throughput(data_by_workload, output_dir):
    """Plot throughput vs threads for different workloads."""

    plt.figure(figsize=(10, 6))

    for workload, data_list in sorted(data_by_workload.items()):
        # Sort by thread count
        data_list.sort(key=lambda x: x['threads'])

        threads = [d['threads'] for d in data_list]
        throughput = [d['throughput'] for d in data_list]

        plt.plot(threads, throughput, marker='o', linewidth=2, markersize=8, label=workload.capitalize())

    plt.xlabel('Number of Clients', fontsize=12, fontweight='bold')
    plt.ylabel('Throughput (requests/sec)', fontsize=12, fontweight='bold')
    plt.title('Throughput vs Number of Clients', fontsize=14, fontweight='bold')
    plt.legend(fontsize=10)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    output_path = os.path.join(output_dir, 'throughput_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def plot_latency(data_by_workload, output_dir, metric='mean'):
    """Plot latency vs threads for different workloads."""

    plt.figure(figsize=(10, 6))

    metric_key = f'{metric}_latency'
    metric_label = metric.upper() if metric in ['p95', 'p99'] else metric.capitalize()

    for workload, data_list in sorted(data_by_workload.items()):
        # Sort by thread count
        data_list.sort(key=lambda x: x['threads'])

        threads = [d['threads'] for d in data_list if metric_key in d]
        latency = [d[metric_key] for d in data_list if metric_key in d]

        if threads:
            plt.plot(threads, latency, marker='o', linewidth=2, markersize=8, label=workload.capitalize())

    plt.xlabel('Number of Clients', fontsize=12, fontweight='bold')
    plt.ylabel(f'{metric_label} Latency (ms)', fontsize=12, fontweight='bold')
    plt.title(f'{metric_label} Latency vs Number of Clients', fontsize=14, fontweight='bold')
    plt.legend(fontsize=10)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    output_path = os.path.join(output_dir, f'{metric}_latency_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def plot_combined_latencies(data_by_workload, output_dir):
    """Plot multiple latency metrics (mean, median, P95, P99) in subplots."""

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    metrics = [('mean', 'Mean'), ('median', 'Median'), ('p95', 'P95'), ('p99', 'P99')]

    for idx, (metric, label) in enumerate(metrics):
        ax = axes[idx // 2, idx % 2]
        metric_key = f'{metric}_latency'

        for workload, data_list in sorted(data_by_workload.items()):
            data_list.sort(key=lambda x: x['threads'])

            threads = [d['threads'] for d in data_list if metric_key in d]
            latency = [d[metric_key] for d in data_list if metric_key in d]

            if threads:
                ax.plot(threads, latency, marker='o', linewidth=2, markersize=6, label=workload.capitalize())

        ax.set_xlabel('Number of Clients', fontsize=10, fontweight='bold')
        ax.set_ylabel(f'{label} Latency (ms)', fontsize=10, fontweight='bold')
        ax.set_title(f'{label} Latency vs Load', fontsize=11, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3)

    plt.tight_layout()
    output_path = os.path.join(output_dir, 'combined_latency_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def print_summary_table(data_by_workload):
    """Print a summary table of all results."""

    print("\n" + "="*80)
    print("LOAD TEST RESULTS SUMMARY")
    print("="*80)

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])

        print(f"\n{workload.upper()} Workload:")
        print("-" * 80)
        print(f"{'Load':<10} {'Throughput':<15} {'Mean Lat':<12} {'P95 Lat':<12} {'P99 Lat':<12}")
        print(f"{'':10} {'(req/s)':<15} {'(ms)':<12} {'(ms)':<12} {'(ms)':<12}")
        print("-" * 80)

        for d in data_list:
            threads = d.get('threads', 'N/A')
            throughput = f"{d.get('throughput', 0):.2f}"
            mean_lat = f"{d.get('mean_latency', 0):.2f}"
            p95_lat = f"{d.get('p95_latency', 0):.2f}"
            p99_lat = f"{d.get('p99_latency', 0):.2f}"

            print(f"{threads:<10} {throughput:<15} {mean_lat:<12} {p95_lat:<12} {p99_lat:<12}")

def main():
    parser = argparse.ArgumentParser(description='Plot load test results')
    parser.add_argument('pattern', help='File pattern (e.g., "read_*thread.txt" or "results/*.txt")')
    parser.add_argument('--output-dir', default='graphs', help='Output directory for graphs')

    args = parser.parse_args()

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Find all matching files
    files = glob.glob(args.pattern)

    if not files:
        print(f"Error: No files found matching pattern: {args.pattern}")
        sys.exit(1)

    print(f"Found {len(files)} log files to process...")

    # Parse all files
    all_data = []
    for filepath in files:
        print(f"  Parsing: {filepath}")
        data = parse_log_file(filepath)
        if data and 'threads' in data and 'throughput' in data:
            all_data.append(data)
        else:
            print(f"    ⚠ Warning: Could not extract required data from {filepath}")

    if not all_data:
        print("Error: No valid data extracted from files")
        sys.exit(1)

    # Group data by workload
    data_by_workload = {}
    for d in all_data:
        workload = d.get('workload', 'unknown')
        if workload not in data_by_workload:
            data_by_workload[workload] = []
        data_by_workload[workload].append(d)

    print(f"\nProcessed {len(all_data)} test results across {len(data_by_workload)} workload(s)")

    # Generate plots
    print("\nGenerating plots...")
    plot_throughput(data_by_workload, args.output_dir)
    plot_latency(data_by_workload, args.output_dir, metric='mean')
    plot_latency(data_by_workload, args.output_dir, metric='median')
    plot_latency(data_by_workload, args.output_dir, metric='p95')
    plot_latency(data_by_workload, args.output_dir, metric='p99')
    plot_combined_latencies(data_by_workload, args.output_dir)

    # Print summary
    print_summary_table(data_by_workload)

    print(f"\n✓ All graphs saved to: {args.output_dir}/")
    print("="*80)

if __name__ == '__main__':
    main()