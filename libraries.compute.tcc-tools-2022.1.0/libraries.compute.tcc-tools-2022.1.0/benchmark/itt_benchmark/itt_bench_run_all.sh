#!/bin/bash

echo "=== CUMULATIVE TESTING ==="
itt_bench_run_cumulative_testing.sh
echo "=== STRANGE TESTING ==="
itt_bench_run_strange_testing.sh
echo "=== BENCHMARK ==="
itt_bench_run_benchmark.sh
echo "=== VALIDATE WITH WORKLOAD ==="
itt_bench_run_workload.sh