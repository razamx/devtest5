Benchmark apply the index of region from lspsram tool.
To identify regions run:
# lspsram
{
  "error":false,
  "data":
  {
    "#0":{"error":false","id":2, "mask":"10001111", "latency":100, "level":"TCC_MEM_DRAM", "size":10000000},
    "#1":{"error":false","id":1, "mask":"10001111", "latency":50, "level":"TCC_MEM_L3", "size":1048576},
    "#2":{"error":false","id":0, "mask":"10001111", "latency":7, "level":"TCC_MEM_L2", "size":262144}
  }
}

Acording to this output you should:
    Use 0 to run with DRAM
    Use 1 to run with L3
    Use 3 to run with l2

