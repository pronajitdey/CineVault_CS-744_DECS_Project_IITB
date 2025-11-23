#!/bin/bash

# Get MovieHTTPServer PID
PID=$(pgrep -f MovieHTTPServer)

if [ -z "$PID" ]; then
    echo "Error: MovieHTTPServer not running"
    exit 1
fi

echo "Monitoring PID: $PID"
echo "Duration: 5 minutes (300 seconds)"
echo "Output directory: ./monitoring_logs"

# Create output directory
mkdir -p monitoring_logs
cd monitoring_logs

# Get timestamp for filenames
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "Starting monitoring at $(date)"

# Start top in background
top -b -d 5 -n 64 -p $PID > top_${TIMESTAMP}.log 2>&1 &
TOP_PID=$!

# Start pidstat in background
pidstat -u -r -d -p $PID 5 64 > pidstat_${TIMESTAMP}.log 2>&1 &
PIDSTAT_PID=$!

# Start iostat in background
iostat -x 5 64 > iostat_${TIMESTAMP}.log 2>&1 &
IOSTAT_PID=$!

# Optional: Monitor network
# sar -n DEV 5 70 > network_${TIMESTAMP}.log 2>&1 &
# SAR_PID=$!

echo "Monitoring processes started:"
echo "  top PID: $TOP_PID"
echo "  pidstat PID: $PIDSTAT_PID"
echo "  iostat PID: $IOSTAT_PID"
# echo "  sar PID: $SAR_PID"

# Wait for all monitoring to complete
wait $TOP_PID $PIDSTAT_PID $IOSTAT_PID #$SAR_PID

echo "Monitoring completed at $(date)"
echo ""
echo "Calculating averages..."

# Calculate and display averages
echo "========== MONITORING SUMMARY =========="

# 1. CPU Usage - Column $8
if [ -f "pidstat_${TIMESTAMP}.log" ]; then
    AVG_CPU=$(awk '
        /%CPU/ || /Average/ { next }
        NF == 10 && $3 ~ /^[0-9]+$/ && $8 ~ /^[0-9]+(\.[0-9]+)?$/ {
            sum += $8
            count++
        }
        END {
            if (count > 0)
                printf "%.2f", sum / count
            else
                print "N/A"
        }
    ' pidstat_${TIMESTAMP}.log)
    
    echo "Average CPU Usage: ${AVG_CPU}%"
    
    if [ "$AVG_CPU" != "N/A" ]; then
        NUM_CORES=$(nproc)
        CORES_USED=$(awk "BEGIN {printf \"%.2f\", $AVG_CPU / 100}")
        echo "Cores Utilized: ${CORES_USED} out of ${NUM_CORES}"
    fi
fi

# 2. Memory Usage - Column $7 in 9-column lines (RSS)
if [ -f "pidstat_${TIMESTAMP}.log" ]; then
    AVG_MEM=$(awk '
        /minflt/ || /majflt/ || /Average/ { next }
        NF == 9 && $3 ~ /^[0-9]+$/ && $7 ~ /^[0-9]+$/ && $7 > 1000 {
            sum += $7
            count++
        }
        END {
            if (count > 0)
                printf "%.2f", sum / count / 1024
            else
                print "N/A"
        }
    ' pidstat_${TIMESTAMP}.log)
    
    echo "Average Memory Usage (RSS): ${AVG_MEM} MB"
fi

# 3. Disk I/O - Already correct
if [ -f "iostat_${TIMESTAMP}.log" ]; then
    # Disk utilization for sdd
    AVG_UTIL=$(awk '
        $1 == "sdd" && NF > 10 {
            sum += $NF
            count++
        }
        END {
            if (count > 0)
                printf "%.4f", sum / count
            else
                print "N/A"
        }
    ' iostat_${TIMESTAMP}.log)
    
    echo "Average Disk Utilization (sdd): ${AVG_UTIL}%"
    
    # I/O await time for sdd
    AVG_AWAIT=$(awk '
        $1 == "sdd" && NF > 10 {
            # Column varies by iostat version
            # Typically await is around column 10-12
            # Look for reasonable await value (0.01 to 10000 ms)
            await_found = 0
            for (i = 6; i <= 15; i++) {
                if ($i ~ /^[0-9]+\.[0-9]+$/ && $i >= 0.01 && $i < 10000) {
                    sum += $i
                    count++
                    await_found = 1
                    break
                }
            }
        }
        END {
            if (count > 0)
                printf "%.4f", sum / count
            else
                print "N/A"
        }
    ' iostat_${TIMESTAMP}.log)
    
    echo "Average I/O Await Time (sdd): ${AVG_AWAIT} ms"
    
    # Additional I/O metrics for sdd
    echo ""
    echo "--- Additional I/O Metrics (sdd) ---"
    
    # Read/Write operations per second
    RW_OPS=$(awk '
        $1 == "sdd" && NF > 10 {
            reads += $2   # r/s column
            writes += $7  # w/s column
            count++
        }
        END {
            if (count > 0)
                printf "Reads: %.2f/s, Writes: %.2f/s", reads/count, writes/count
            else
                print "N/A"
        }
    ' iostat_${TIMESTAMP}.log)
    echo "  ${RW_OPS}"
    
    # KB read/written per second
    RW_KB=$(awk '
        $1 == "sdd" && NF > 10 {
            read_kb += $3   # rkB/s column
            write_kb += $8  # wkB/s column
            count++
        }
        END {
            if (count > 0)
                printf "Read: %.2f KB/s, Write: %.2f KB/s", read_kb/count, write_kb/count
            else
                print "N/A"
        }
    ' iostat_${TIMESTAMP}.log)
    echo "  ${RW_KB}"
    
    # Peak utilization
    MAX_UTIL=$(awk '
        $1 == "sdd" && NF > 10 {
            if ($NF > max) max = $NF
        }
        END {
            if (max > 0)
                printf "%.2f", max
            else
                print "N/A"
        }
    ' iostat_${TIMESTAMP}.log)
    echo "  Peak Utilization: ${MAX_UTIL}%"
fi

echo "========================================"
echo ""
echo "Detailed logs saved in monitoring_logs/"

# Create summary file
cat > summary_${TIMESTAMP}.txt <<EOF
Monitoring Summary - $(date)
================================
PID Monitored: $PID
Duration: 5 minutes (300 seconds)

CPU Usage: ${AVG_CPU}%
Memory Usage: ${AVG_MEM} MB
Disk Utilization: ${AVG_UTIL}%
I/O Await Time: ${AVG_AWAIT} ms
Peak Utilization: ${MAX_UTIL}%

Raw logs:
- top_${TIMESTAMP}.log
- pidstat_${TIMESTAMP}.log
- iostat_${TIMESTAMP}.log
EOF

echo "Summary saved to summary_${TIMESTAMP}.txt"
