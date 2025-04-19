#! /bin/bash
#
# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#
# Script for generating flame graphs for KV service without package manager dependencies

echo "Checking existing system tools..."

# Check if perf is installed
if ! which perf &>/dev/null; then
  echo "Warning: perf tool not found, will use alternative methods"
  HAS_PERF=false
else
  echo "Found perf tool"
  HAS_PERF=true
fi

# Check if git is available
if ! which git &>/dev/null; then
  echo "Warning: git not found, will try direct download of flame graph scripts"
  HAS_GIT=false
else
  echo "Found git tool"
  HAS_GIT=true
fi

# Get FlameGraph scripts
if [ ! -d "FlameGraph" ]; then
  if [ "$HAS_GIT" = true ]; then
    echo "Cloning FlameGraph repository..."
    git clone https://github.com/brendangregg/FlameGraph.git
  else
    echo "Downloading necessary scripts directly..."
    mkdir -p FlameGraph
    curl -s -o FlameGraph/stackcollapse-perf.pl https://raw.githubusercontent.com/brendangregg/FlameGraph/master/stackcollapse-perf.pl
    curl -s -o FlameGraph/flamegraph.pl https://raw.githubusercontent.com/brendangregg/FlameGraph/master/flamegraph.pl
    chmod +x FlameGraph/*.pl
  fi
else
  echo "FlameGraph directory already exists"
fi

# Step 2: Find server process
echo "Finding server process..."
SERVER_PROCESS="kv_server"
SERVER_PID=$(pgrep -f "$SERVER_PROCESS")
TEST_DURATION=30

if [ -z "$SERVER_PID" ]; then
  echo "KV server process not found, please enter exact process name or PID:"
  read -p "Process name or PID: " SERVER_PROCESS
  if [[ "$SERVER_PROCESS" =~ ^[0-9]+$ ]]; then
    SERVER_PID=$SERVER_PROCESS
  else
    SERVER_PID=$(pgrep -f "$SERVER_PROCESS")
  fi

  if [ -z "$SERVER_PID" ]; then
    echo "Still unable to find server process. Here are potential related processes:"
    ps aux | grep -i "kv\|server"
    exit 1
  fi
fi

echo "Server process found, PID: $SERVER_PID"

# Step 3: Collect performance data
if [ "$HAS_PERF" = true ]; then
  echo "Using perf to collect performance data for $TEST_DURATION seconds..."
  sudo perf record -F 99 -p $SERVER_PID -g -- sleep $TEST_DURATION

  # Check if perf record was successful
  if [ ! -f "perf.data" ]; then
    echo "perf record did not generate data file, trying without -g parameter..."
    sudo perf record -F 99 -p $SERVER_PID -- sleep $TEST_DURATION
  fi

  # If still failing, try using gdb
  if [ ! -f "perf.data" ]; then
    echo "perf tool failed, trying alternative methods..."
    HAS_PERF=false
  else
    echo "Processing perf data..."
    sudo perf script > perf.out
  fi
fi

# If perf is unavailable or failed, use gdb or system tools
if [ "$HAS_PERF" = false ]; then
  echo "Using alternative methods to collect performance data..."

  # Check if gdb is available
  if which gdb &>/dev/null; then
    echo "Using gdb to collect stack information..."
    STACKS_FILE="stack_traces.txt"
    > $STACKS_FILE  # Clear file

    for i in $(seq 1 10); do
      echo "Collecting sample $i/10..."
      sudo gdb -p $SERVER_PID -batch -ex "thread apply all bt" >> $STACKS_FILE 2>/dev/null
      sleep $(($TEST_DURATION / 10))
    done

    # Simple processing into format usable for flame graph
    cat $STACKS_FILE | grep "^#" | sed 's/^#\([0-9]*\)[ \t]*\(.*\)$/\2 1/g' > perf.folded
  else
    echo "gdb not found, can only collect basic information using top and ps..."
    TOP_FILE="top_output.txt"
    PS_FILE="ps_output.txt"

    top -b -n 5 -d 3 -p $SERVER_PID > $TOP_FILE
    ps -o pid,ppid,cmd,%cpu,%mem,vsz,rss,stat -p $SERVER_PID > $PS_FILE

    echo "Cannot generate flame graph, but basic performance data has been collected in $TOP_FILE and $PS_FILE"
    exit 1
  fi
fi

# Step 4: Generate flame graph
if [ -f "perf.out" ]; then
  echo "Generating flame graph from perf data..."
  ./FlameGraph/stackcollapse-perf.pl perf.out > perf.folded
elif [ ! -f "perf.folded" ]; then
  echo "No usable performance data found, cannot generate flame graph"
  exit 1
fi

if [ -f "perf.folded" ]; then
  # Check for perl
  if ! which perl &>/dev/null; then
    echo "Error: perl not found, cannot generate flame graph"
    exit 1
  fi

  ./FlameGraph/flamegraph.pl perf.folded > kv_service_flamegraph.svg
  echo "Flame graph generated: kv_service_flamegraph.svg"
fi
