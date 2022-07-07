# tcc_multiple_measurements_sample

This sample is a simple example how to run multiple measurements on several workloads in one application.
In this sample you can see how to instrument the code with Instrumentation and Tracing Technology API (ITT API) for multiple workloads, including nested workloads.
This measurement sample is a building block of the ``tcc_measurement_analysis_sample`` demonstration.

Sample has two workloads: matrix multiplication and 2/PI approximation. 
The purprose of those workloads is to show how measurements collection works on basic computations.

Measurements collected in the sample are for matrix multiplication function, 2/PI approximation function, and overall measurement for both functions.

**Note:** Samples are intended to demonstrate how to write code using certain features. They are not performance benchmarks. Example output shown here is for illustration only. Your output may vary.

## Source Files

 File | Description
 ---- | -----------
 [main.c](src/main.c)| Main file containing sample
 
## Example Command

```
tcc_multiple_measurements_sample -a 100 -m 100 -i 100
```

## Example Output

```
Running with arguments:
    approximation = 100,
    multiplication = 100,
    iterations = 100
Running workloads. This may take a while, depending on iteration values.
Workloads were run successfully.
```

Output of this sample simply shows arguments and execution status.
To see more information about analyzing the profiling results, please read documentation for ``tcc_measurement_analysis_sample``.

## Command-Line Options
```
Usage: tcc_multiple_measurements_sample -a N -m N [-i N]
Options:
    -a | --approximation N     Required. Execute 2/PI approximation workload with N number.
    -m | --multiplication N    Required. Execute matrices multiplication workload with N number.
    -i | --iterations N        Execute N iterations of the main loop to gather
                               more precise timing statistics. Default: 1,000.
    -h | --help                Show this help message and exit.

```

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
