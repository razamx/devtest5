# tcc_cache_allocation_sample

tcc_cache_allocation_sample is a simple self-contained example program demonstrating use of the cache allocation library.

The sample is intended to demonstrate usage of the library on all platforms.

**Note:** Samples are intended to demonstrate how to write code using certain features.
They are not performance benchmarks. Example output shown here is for illustration only. Your output may vary.

## Source Files

 File | Description
 ---- | -----------
 [main.c](src/main.c)| Main file containing simple cache allocation sample code.

## Allocate a Buffer

The following command shows basic usage of the sample.

### Example Command for L2 Cache (Internal Noisy Neighbor)

For Elkhart Lake, use `--latency 250`
```
tcc_cache_allocation_sample --collect --latency 80 --stress
```
### Example Output
```
Running with arguments:
    cpuid = 3,
    latency = 80 ns,
    stress = internal,
    iterations = 100,
    collector = libtcc_collector.so
Allocating memory according to the latency requirements
Running workload. This may take a while, depending on sleep and iteration values
Deallocating memory

*** Statistics for workload ****************************
   Minimum total latency: 8240 CPU cycles (3301 ns)
   Maximum total latency: 10677 CPU cycles (4277 ns)
   Average total latency: 9082 CPU cycles (3638 ns)

   Minimum latency per buffer access: 16 CPU cycles (6 ns)
   Maximum latency per buffer access: 21 CPU cycles (8 ns)
   Average latency per buffer access: 18 CPU cycles (7 ns)
********************************************************
```

### Example Command for L2 Cache (External Noisy Neighbor)

1. Open two terminal windows.
2. In the first terminal window, run the Linux* tool stress-ng as a noisy neighbor:

```
stress-ng -t 0 -C 10 --cache-level 2 --taskset 2 --aggressive
```

3. In the second terminal window, run the sample:

For Elkhart Lake, use `--latency 250`
```
tcc_cache_allocation_sample --collect --latency 80 --sleep 10000000
```

### Example Output
```
Running with arguments:
    cpuid = 3,
    latency = 80 ns,
    stress = external (sleep 10000000 ns),
    iterations = 100,
    collector = libtcc_collector.so
Allocating memory according to the latency requirements
Running workload. This may take a while, depending on sleep and iteration values

Deallocating memory

*** Statistics for workload ****************************
   Minimum total latency: 8240 CPU cycles (3301 ns)
   Maximum total latency: 10677 CPU cycles (4277 ns)
   Average total latency: 9082 CPU cycles (3638 ns)

   Minimum latency per buffer access: 16 CPU cycles (6 ns)
   Maximum latency per buffer access: 21 CPU cycles (8 ns)
   Average latency per buffer access: 18 CPU cycles (7 ns)
********************************************************
```
### Example Command for L3 Cache (Internal Noisy Neighbor)
```
tcc_cache_allocation_sample --collect --latency 300 --stress
```
### Example Output
```
Running with arguments:
    cpuid = 3,
    latency = 300 ns,
    stress = internal,
    iterations = 100,
    collector = libtcc_collector.so
Allocating memory according to the latency requirements
Running workload. This may take a while, depending on sleep and iteration values
Deallocating memory

*** Statistics for workload ****************************
    Minimum total latency: 5620 CPU cycles (3752 ns)
    Maximum total latency: 19833 CPU cycles (13243 ns)
    Average total latency: 6751 CPU cycles (4507 ns)

    Minimum latency per buffer access: 11 CPU cycles (7 ns)
    Maximum latency per buffer access: 39 CPU cycles (25 ns)
    Average latency per buffer access: 13 CPU cycles (8 ns)
********************************************************
```

### Example Command for L3 Cache (External Noisy Neighbor)

1. Open two terminal windows.
2. In the first terminal window, run the Linux* tool stress-ng as a noisy neighbor:

```
stress-ng -t 0 -C 10 --cache-level 3 --taskset 2 --aggressive
```

3. In the second terminal window, run the sample:

```
tcc_cache_allocation_sample --collect --latency 300 --sleep 10000000
```
### Example Output
```
Running with arguments:
    cpuid = 3,
    latency = 300 ns,
    stress = external (sleep 10000000 ns),
    iterations = 100,
    collector = libtcc_collector.so
Allocating memory according to the latency requirements
Running workload. This may take a while, depending on sleep and iteration values
Deallocating memory

*** Statistics for workload ****************************
    Minimum total latency: 1995 CPU cycles (1332 ns)
    Maximum total latency: 20134 CPU cycles (13444 ns)
    Average total latency: 5872 CPU cycles (3920 ns)

    Minimum latency per buffer access: 4 CPU cycles (2 ns)
    Maximum latency per buffer access: 39 CPU cycles (26 ns)
    Average latency per buffer access: 11 CPU cycles (7 ns)
********************************************************

```

## Command-Line Options

```
Usage: tcc_cache_allocation_sample -l N [--sleep N | --stress | --nostress]
                       [-p N] [-c] [-i N]

Options:
    --sleep N | --stress | --nostress
    Select one of these options:
    --sleep N                  Period between iterations in N nanoseconds.
                               Select this option when you want to run
                               your own noisy neighbor.
    --stress                   Run the sample's provided noisy neighbor
                               on core 3 (default).
    --nostress                 Do not run the sample's provided noisy neighbor,
                               and run workload iterations continuously
                               without sleep in between.

    -p | --cpuid N             Specify the requested core. Default: 3.
    -l | --latency N           Specify the maximum tolerable latency
                               for a single cache line access in N nanoseconds.
    -i | --iterations N        Execute N iterations of the main loop to gather
                               more precise timing statistics. Default: 100.
    -c | --collect             Enable measurement result collection if
                               INTEL_LIBITTNOTIFY64 was not set via environment.
    -h | --help                Show this help message and exit.

```

## Cache Allocation Library Code Example

tcc_cache_allocation_sample allocates and uses a memory buffer using tcc_cache_malloc().

### Initializing Memory Buffer and Allocating Memory

The library allocates buffers based on the latency requirement and
underlying platform capabilities. For low-latency buffers, it allocates buffers
in Level 2 (L2) or Level 3 (L3) cache. For more
information, see the API Reference.

```
	/* Bind a process to the CPU specified by the cpuid variable (set process affinity) */
	PRINT_ERROR_EXIT(tcc_cache_init(CPUID));

	/* Allocates memory according to the latency requirement */
	void *mem = tcc_cache_malloc(BUFFER_SIZE, latency);

	/* Run the application workload */
	/* Evaluate memory access latency */
    run_workload_and_measure_cycles(mem, WORKLOAD_BUFFER_SIZE, sample_setting.iterations, sample_setting);

```

### Releasing Memory and Destroying Buffer


```
	/* Releasing memory */
	tcc_cache_free(mem);
	/* Finish working with tcc libraries */
	tcc_sts = tcc_cache_finish();
```
## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
