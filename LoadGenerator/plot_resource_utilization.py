#!/usr/bin/env python3
"""
Resource Utilization Plotter
Creates CPU, Disk, and I/O utilization graphs from monitoring summary files.

Usage:
    python plot_resource_utilization.py <summary_pattern> [--output-dir DIR] [--cores N]

Example:
    python plot_resource_utilization.py "summary_*thread.txt" --output-dir graphs/ --cores 8
"""

import os
import re
import sys
import glob
import argparse
import matplotlib.pyplot as plt
import numpy as np

def parse_summary_file(filepath):
    """Parse summary monitoring file for resource utilization."""
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
            # Try from content
            thread_match = re.search(r'(\d+)\s*threads?', content, re.IGNORECASE)
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

        # Extract CPU usage
        cpu_match = re.search(r'CPU Usage:\s*([\d.]+)%', content)
        data['cpu_usage'] = float(cpu_match.group(1)) if cpu_match else 0

        # Extract Memory usage
        mem_match = re.search(r'Memory Usage:\s*([\d.]+)\s*MB', content)
        data['memory_usage'] = float(mem_match.group(1)) if mem_match else 0

        # Extract Disk utilization
        disk_match = re.search(r'Disk Utilization:\s*([\d.]+)%', content)
        data['disk_util'] = float(disk_match.group(1)) if disk_match else 0

        # Extract I/O await time
        await_match = re.search(r'I/O Await Time:\s*([\d.]+)\s*ms', content)
        data['io_await'] = float(await_match.group(1)) if await_match else 0

        return data

    except Exception as e:
        print(f"  ✗ Error parsing {filepath}: {e}")
        return None

def plot_cpu_utilization(data_by_workload, output_dir, num_cores=8):
    """Plot CPU utilization vs threads with 100% scale."""
    plt.figure(figsize=(10, 6))

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if 'cpu_usage' in d and d['cpu_usage'] > 0]
        cpu_usage = [d['cpu_usage'] for d in data_list if 'cpu_usage' in d and d['cpu_usage'] > 0]

        if threads:
            plt.plot(threads, cpu_usage, marker='o', linewidth=2.5, markersize=8, label=f'{workload} Workload')

    # Add bottleneck threshold lines
    plt.axhline(y=80, color='orange', linestyle='--', linewidth=2, alpha=0.7, label='High Load (80%)')
    plt.axhline(y=90, color='red', linestyle='--', linewidth=2, alpha=0.7, label='CPU Bottleneck (90%)')

    plt.xlabel('Number of Clients', fontsize=13, fontweight='bold')
    plt.ylabel('CPU Utilization (%)', fontsize=13, fontweight='bold')
    plt.title('CPU Utilization vs Number of Clients', fontsize=15, fontweight='bold', pad=20)
    plt.ylim(0, 100)  # Fixed scale 0-100%
    plt.xlim(left=0)
    plt.legend(fontsize=11, loc='best')
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.tight_layout()

    output_path = os.path.join(output_dir, 'cpu_utilization_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def plot_disk_utilization(data_by_workload, output_dir):
    """Plot disk utilization vs threads with 100% scale."""
    plt.figure(figsize=(10, 6))

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if 'disk_util' in d]
        disk_util = [d['disk_util'] for d in data_list if 'disk_util' in d]

        if threads:
            plt.plot(threads, disk_util, marker='s', linewidth=2.5, markersize=8, label=f'{workload} Workload')

    # Add bottleneck threshold lines
    plt.axhline(y=50, color='yellow', linestyle='--', linewidth=2, alpha=0.7, label='Moderate Load (50%)')
    plt.axhline(y=80, color='orange', linestyle='--', linewidth=2, alpha=0.7, label='High Load (80%)')
    plt.axhline(y=90, color='red', linestyle='--', linewidth=2, alpha=0.7, label='I/O Bottleneck (90%)')

    plt.xlabel('Number of Clients', fontsize=13, fontweight='bold')
    plt.ylabel('Disk Utilization (%)', fontsize=13, fontweight='bold')
    plt.title('Disk Utilization vs Number of Clients', fontsize=15, fontweight='bold', pad=20)
    plt.ylim(0, 100)  # Fixed scale 0-100%
    plt.xlim(left=0)
    plt.legend(fontsize=11, loc='best')
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.tight_layout()

    output_path = os.path.join(output_dir, 'disk_utilization_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def plot_io_await(data_by_workload, output_dir):
    """Plot I/O await time vs threads."""
    plt.figure(figsize=(10, 6))

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if 'io_await' in d]
        io_await = [d['io_await'] for d in data_list if 'io_await' in d]

        if threads:
            plt.plot(threads, io_await, marker='^', linewidth=2.5, markersize=8, label=f'{workload} Workload')

    plt.xlabel('Number of Clients', fontsize=13, fontweight='bold')
    plt.ylabel('I/O Await Time (ms)', fontsize=13, fontweight='bold')
    plt.title('I/O Await Time vs Number of Clients', fontsize=15, fontweight='bold', pad=20)
    plt.xlim(left=0)
    plt.ylim(bottom=0)
    plt.legend(fontsize=11, loc='best')
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.tight_layout()

    output_path = os.path.join(output_dir, 'io_await_vs_load.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def plot_bottleneck_dashboard(data_by_workload, output_dir):
    """Create a 2x2 dashboard with all resource metrics."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))

    # Plot 1: CPU Utilization (top-left)
    ax = axes[0, 0]
    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if 'cpu_usage' in d]
        cpu = [d['cpu_usage'] for d in data_list if 'cpu_usage' in d]
        if threads:
            ax.plot(threads, cpu, marker='o', linewidth=2, markersize=6, label=f'{workload} Workload')
    ax.axhline(y=80, color='orange', linestyle='--', alpha=0.6, linewidth=1.5)
    ax.axhline(y=90, color='red', linestyle='--', alpha=0.6, linewidth=1.5, label='Bottleneck (90%)')
    ax.set_xlabel('Number of Clients', fontweight='bold')
    ax.set_ylabel('CPU Usage (%)', fontweight='bold')
    ax.set_title('CPU Utilization', fontweight='bold', fontsize=12)
    ax.set_ylim(0, 100)
    ax.set_xlim(left=0)
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    # Plot 2: Disk Utilization (top-right)
    ax = axes[0, 1]
    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if 'disk_util' in d]
        disk = [d['disk_util'] for d in data_list if 'disk_util' in d]
        if threads:
            ax.plot(threads, disk, marker='s', linewidth=2, markersize=6, label=f'{workload} Workload')
    ax.axhline(y=80, color='orange', linestyle='--', alpha=0.6, linewidth=1.5)
    ax.axhline(y=90, color='red', linestyle='--', alpha=0.6, linewidth=1.5, label='Bottleneck (90%)')
    ax.set_xlabel('Number of Clients', fontweight='bold')
    ax.set_ylabel('Disk Utilization (%)', fontweight='bold')
    ax.set_title('Disk I/O Utilization', fontweight='bold', fontsize=12)
    ax.set_ylim(0, 100)
    ax.set_xlim(left=0)
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    # Plot 3: I/O Await Time (bottom-left)
    ax = axes[1, 0]
    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if 'io_await' in d]
        await_time = [d['io_await'] for d in data_list if 'io_await' in d]
        if threads:
            ax.plot(threads, await_time, marker='^', linewidth=2, markersize=6, label=f'{workload} Workload')
    ax.set_xlabel('Number of Clients', fontweight='bold')
    ax.set_ylabel('I/O Await Time (ms)', fontweight='bold')
    ax.set_title('I/O Wait Time', fontweight='bold', fontsize=12)
    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    # Plot 4: Memory Usage (bottom-right)
    ax = axes[1, 1]
    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])
        threads = [d['threads'] for d in data_list if 'memory_usage' in d]
        memory = [d['memory_usage'] for d in data_list if 'memory_usage' in d]
        if threads:
            ax.plot(threads, memory, marker='d', linewidth=2, markersize=6, label=f'{workload} Workload')
    ax.set_xlabel('Number of Clients', fontweight='bold')
    ax.set_ylabel('Memory Usage (MB)', fontweight='bold')
    ax.set_title('Memory Consumption', fontweight='bold', fontsize=12)
    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    plt.suptitle('Resource Utilization and Bottleneck Analysis Dashboard', 
                 fontsize=16, fontweight='bold', y=0.995)
    plt.tight_layout()

    output_path = os.path.join(output_dir, 'bottleneck_dashboard.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_path}")
    plt.close()

def print_summary(data_by_workload):
    """Print summary of resource utilization."""
    print("\n" + "="*70)
    print("RESOURCE UTILIZATION SUMMARY")
    print("="*70)

    for workload, data_list in sorted(data_by_workload.items()):
        data_list.sort(key=lambda x: x['threads'])

        print(f"\n{workload} Workload:")
        print("-" * 70)
        print(f"{'Load':<10} {'CPU %':<10} {'Disk %':<10} {'I/O Wait':<12} {'Memory':<10}")
        print(f"{'':10} {'':10} {'':10} {'(ms)':<12} {'(MB)':<10}")
        print("-" * 70)

        for d in data_list:
            threads = d.get('threads', 'N/A')
            cpu = f"{d.get('cpu_usage', 0):.2f}"
            disk = f"{d.get('disk_util', 0):.4f}"
            io_wait = f"{d.get('io_await', 0):.4f}"
            memory = f"{d.get('memory_usage', 0):.2f}"

            print(f"{threads:<10} {cpu:<10} {disk:<10} {io_wait:<12} {memory:<10}")

def main():
    parser = argparse.ArgumentParser(
        description='Plot resource utilization graphs from monitoring summary files'
    )
    parser.add_argument('pattern', help='Summary file pattern (e.g., "summary_*thread.txt")')
    parser.add_argument('--output-dir', default='graphs', help='Output directory for graphs')
    parser.add_argument('--cores', type=int, default=8, help='Number of CPU cores (for reference)')

    args = parser.parse_args()

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Find summary files
    print("\n" + "="*70)
    print("RESOURCE UTILIZATION PLOTTER")
    print("="*70)

    summary_files = glob.glob(args.pattern)
    if not summary_files:
        print(f"\nError: No files found matching pattern: {args.pattern}")
        sys.exit(1)

    print(f"\nFound {len(summary_files)} summary files")

    # Parse all summary files
    data_by_workload = {}
    for filepath in summary_files:
        print(f"  Parsing: {filepath}")
        data = parse_summary_file(filepath)
        if data and 'threads' in data:
            workload = data.get('workload', 'Unknown')
            if workload not in data_by_workload:
                data_by_workload[workload] = []
            data_by_workload[workload].append(data)

    if not data_by_workload:
        print("\nError: No valid data extracted from files")
        sys.exit(1)

    print(f"\nProcessed {sum(len(v) for v in data_by_workload.values())} data points")
    print(f"Workloads found: {', '.join(data_by_workload.keys())}")

    # Generate graphs
    print("\n" + "="*70)
    print("GENERATING RESOURCE UTILIZATION GRAPHS")
    print("="*70)
    print()

    plot_cpu_utilization(data_by_workload, args.output_dir, args.cores)
    plot_disk_utilization(data_by_workload, args.output_dir)
    plot_io_await(data_by_workload, args.output_dir)

    print("\n" + "="*70)
    print("GENERATING BOTTLENECK ANALYSIS DASHBOARD")
    print("="*70)
    print()

    plot_bottleneck_dashboard(data_by_workload, args.output_dir)

    # Print summary
    print_summary(data_by_workload)

    print("\n" + "="*70)
    print("ALL GRAPHS GENERATED SUCCESSFULLY")
    print("="*70)
    print(f"\nGraphs saved to: {args.output_dir}/")
    print("\nGenerated Files:")
    print("  • cpu_utilization_vs_threads.png")
    print("  • disk_utilization_vs_threads.png")
    print("  • io_await_vs_threads.png")
    print("  • bottleneck_dashboard.png")
    print("\n" + "="*70)

if __name__ == '__main__':
    main()