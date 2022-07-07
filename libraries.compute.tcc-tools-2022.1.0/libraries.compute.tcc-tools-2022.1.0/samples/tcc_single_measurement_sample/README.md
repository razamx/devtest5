# tcc_single_measurement_sample

tcc_single_measurement_sample is a simple self-contained example program demonstrating use of the measurement library.

**Note:** Samples are intended to demonstrate how to write code using certain features.
They are not performance benchmarks. Example output shown here is for illustration only. Your output may vary.

## Source Files

 File | Description
 ---- | -----------
 [main.c](src/main.c)| Main file containing sample code.

## Examples of Usage

| [Without emulation of deadline violations](#without-emulation-of-deadline-violations-with-artificial-outliers) | [With emulation of deadline violations](#with-emulation-of-deadline-violations-with-artificial-outliers) |

### Without Emulation of Deadline Violations with Artificial Outliers

In this mode, the sample will run without emulation of artificial outliers. As a result, deadline violations may not occur. The deadline violations in this mode depend on the load of your test system.

#### Example Command

tcc_single_measurement_sample -a 10 -d 2000,ns

#### Example Output

```
Running with arguments:
    approximation = 10,
    iterations = 1000,
    outliers = False,
    deadline = 2000 ns,
Approximation #10 is:0.636620
[Approximation] Iterations 1000; iteration duration [ns]: avg=133.000 min=131.000 max=481.000 jitter=349.000
Number of exceeding deadlines: 0 of 1000

```

### With Emulation of Deadline Violations with Artificial Outliers

To see how the example behaves when the deadlines are exceeded,
you can use the ``--emulate-outliers`` (``-o``) option, which will use a heavier workload for 20% of the measurements. This will lead to deadline violations.

#### Example Command

tcc_single_measurement_sample -a 10 -d 2000,ns -i 25 -o

#### Example Output

```
Running with arguments:
    approximation = 10,
    iterations = 25,
    outliers = True,
    deadline = 2000 ns,
Latency exceeding deadline: 44820 CPU cycles (15988 nsec) (15 usec)
Latency exceeding deadline: 83950 CPU cycles (29947 nsec) (29 usec)
Latency exceeding deadline: 42502 CPU cycles (15161 nsec) (15 usec)
Latency exceeding deadline: 64778 CPU cycles (23108 nsec) (23 usec)
Latency exceeding deadline: 66416 CPU cycles (23692 nsec) (23 usec)
Approximation #10 is:0.636620
[Approximation] Iterations 25; iteration duration [ns]: avg=4433.000 min=134.000 max=29947.000 jitter=29813.000
Number of exceeding deadlines: 5 of 25

```

## Command-Line Options

```
Usage: tcc_single_measurement_sample -a N [-i N] [-d N<,clk|,ns|,us>] [-o]
Options:
    -a | --approximation N          Required. Calculates the Nth approximation of 2/pi.
    -i | --iterations N             Execute N iterations of the main loop to gather
                                    more precise timing statistics. Default: 1,000.
    -d | --deadline N<,clk|,ns|,us> Specify the maximum tolerable latency
                                    for each iteration in N CPU clock cycles (clk),
                                    nanoseconds (ns), or microseconds (us).
                                    Default: 0 (no deadline).
    -o | --emulate-outliers         Enable emulation of outliers (amount about 20 percent).
    -h | --help                     Show this help message and exit.
```

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
