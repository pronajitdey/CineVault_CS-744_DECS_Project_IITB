# CineVault Load Generator

A comprehensive, multi-threaded HTTP load generator for testing the CineVault Movie Store System. This tool helps identify performance bottlenecks and system capacity under various workload patterns.

## Features

**Multiple Workload Types**

- `read` - Read-heavy workload (70% list, 30% search)
- `write` - Write-heavy workload (80% add, 15% update, 5% delete)
- `mixed` - Configurable mix of operations
- `search` - Search-intensive workload
- `update` - Update-intensive workload

**Detailed Metrics**

- Request throughput (requests/second)
- Success/failure rates
- Latency statistics (mean, median, P95, P99, min, max)
- Per-operation breakdown (ADD, LIST, SEARCH, UPDATE, DELETE)
- CSV export for detailed analysis

**Advanced Configuration**

- Configurable thread count
- Warmup period support
- Think time between requests
- Custom workload ratios
- Test duration control

## Build Instructions

```bash
cd LoadGenerator
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage

### Basic Usage

```bash
# Run with default settings (mixed workload, 4 threads, 60s)
./LoadGenerator

# Show help
./LoadGenerator --help
```

## Command-Line Options

| Option                  | Description                                    | Default       |
| ----------------------- | ---------------------------------------------- | ------------- |
| `--host <hostname>`     | Server hostname                                | localhost     |
| `--port <port>`         | Server port                                    | 8080          |
| `--threads <num>`       | Number of concurrent threads                   | 4             |
| `--duration <seconds>`  | Test duration in seconds                       | 60            |
| `--warmup <seconds>`    | Warmup period in seconds                       | 10            |
| `--workload <type>`     | Workload type (read/write/mixed/search/update) | mixed         |
| `--read-ratio <ratio>`  | Read operation ratio (for mixed workload)      | 0.7           |
| `--write-ratio <ratio>` | Write operation ratio (for mixed workload)     | 0.2           |
| `--think-time <ms>`     | Think time between requests in milliseconds    | 0             |
| `--output <filename>`   | CSV output filename for latency data           | latencies.csv |
| `--help`                | Show help message                              | -             |

## Workload Types Explained

### 1. Read-Heavy Workload (`--workload read`)

**Purpose:** Test cache effectiveness and read scalability

- 70% LIST operations (retrieve all movies)
- 30% SEARCH operations (find specific movies)
- **Bottleneck Expected:** CPU utilization, database read performance

### 2. Write-Heavy Workload (`--workload write`)

**Purpose:** Test database write performance and transaction handling

- 80% ADD operations (insert new movies)
- 15% UPDATE operations (modify ratings)
- 5% DELETE operations (remove movies)
- **Bottleneck Expected:** Database write performance, I/O operations

### 3. Mixed Workload (`--workload mixed`)

**Purpose:** Realistic simulation with configurable operation mix

- Configurable ratios via `--read-ratio` and `--write-ratio`
- Default: 70% reads, 20% writes, 10% searches
- **Bottleneck Expected:** Varies based on configuration

### 4. Search-Intensive (`--workload search`)

**Purpose:** Test search query performance

- 100% SEARCH operations
- **Bottleneck Expected:** Database query optimization, index usage

### 5. Update-Intensive (`--workload update`)

**Purpose:** Test concurrent update handling

- 100% UPDATE operations
- **Bottleneck Expected:** Database lock contention, transaction serialization

## Example Test Scenarios for CS744 Project

### Scenario 1: Identify Cache Effectiveness

```bash
./LoadGenerator --workload read --threads 4 --duration 300 --output read_test.csv
```

### Scenario 2: Identify Write Bottleneck

```bash
./LoadGenerator --workload write --threads 4 --duration 300 --output write_test.csv
```

## Output Interpretation

### Console Output

```
========== LOAD TEST RESULTS ==========
Duration: 300 seconds
Total Requests: 9334
Successful (200): 9206
Not Found (500): 0
Failed: 128

Success Rates:
  Actual Operations: 98.63%
  Server Responses: 98.63%

Throughput:
  Total: 31.11 req/s
  Effective: 30.69 req/s

Request Distribution:
  ADD: 0
  LIST: 6670
  SEARCH: 2664
  UPDATE: 0
  DELETE: 0

Latency Statistics (ms):
  Mean: 902.01
  Median: 942.81
  P95: 1218.45
  P99: 1505.20
  Min: 278.13
  Max: 7071.69
=======================================
```

### CSV Output

The tool exports detailed latency data to `latencies.csv` (or specified file):

```csv
request_id,latency_ms
0,4.23
1,3.45
2,5.67
...
```

Data used for:

- Plotting latency distributions
- Time-series analysis
- Identifying performance degradation over time

## Performance Metrics to Analyze

1. **Throughput (req/s)**

   - Measure of system capacity
   - Compare across different workloads
   - Identify saturation point

2. **Latency (ms)**

   - Mean: Average response time
   - Median (P50): Typical user experience
   - P95/P99: Tail latency (worst-case scenarios)

3. **Success Rate**

   - Successful / Total Requests
   - Identifies stability under load
   - Track connection timeouts

4. **Cache Hit Rate**
   - Compare read performance before/after warmup
   - Analyze LIST operation latency distribution
