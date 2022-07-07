# tcc_measurement_analysis_sample

tcc_measurement_analysis_sample is an example of a profiling
application. The sample demonstrates the following types of analysis:

* Creating histograms of collected data
* Monitoring measurements and deadline violations

This sample is a universal instrument for measurement library data analysis, written in Python for convenience, and
can be used as is to analyze real data from any application instrumented with the measurement library.

**Note:** Samples are intended to demonstrate how to write code using certain features. They are not performance benchmarks. Example output shown here is for illustration only. Your output may vary.

## Source Files

 File | Description
 ---- | -----------
 [tcc_measurement_analysis_sample.py](src/tcc_measurement_analysis_sample.py)| Main file containing sample code

## Examples of Usage

| [Create a Histogram](#create-a-histogram) | [Monitoring Measurements](#monitor-measurements) |

### Create a Histogram

You can create a histogram based on data collected by the measurement library during application run.
The sample creates histograms for the specified measurement instances.

#### Command-Line Usage

~~~
usage: tcc_measurement_analysis_sample hist [<measurement_instances>] [-dump-file] [-time-units] [-nn cpu] -- <app> [<args>]

command arguments:
  measurement_instances
                        Specify the measurement_instances inside the profiled application to use to collect the measurement history.
                        This argument can be omitted if environment variable TCC_MEASUREMENTS_BUFFERS is specified
                        (TCC_MEASUREMENTS_BUFFERS serves the same purpose). This argument overrides TCC_MEASUREMENTS_BUFFERS
                        environment variable.
                        Format: <measurement_instance_name>:<buffer_size>[,<measurement_instance_name>:<buffer_size> ...],
                        where the measurement instances name is the measurement name passed to __itt_string_handle_create()
                        in the profiled application, and the buffer size is the maximum number of measurements that the buffer can store.
                        Specifying too high a value will require lots of memory for the profiled application.
                        Example: "Cycle:1000,Sensor:1000"
                        NOTE: If the buffer size defined in the measurement_instances argument does not match the total number of
                        measurements (that is, only N last samples are collected), the full data statistics
                        (average, maximum, and minimum) may not match the histogram statistics.
  app                   Specify the path to the profiled application (example: ./tcc_multiple_measurements_sample).
  args                  Command-line arguments for the application being profiled.

options:
  -h, --help            Show this help message and exit
  -dump-file file       Specify the history dump file path from the profiled application.
                        The file contains measurements collected by the measurement library during application run.
                        This option overrides TCC_MEASUREMENTS_DUMP_FILE environment variable
  -time-units units     Specify time units
                        "ns" for nanoseconds
                        "us" for microseconds
                        "clk" for CPU clock cycles (default)
                        This option overrides TCC_MEASUREMENTS_TIME_UNIT environment variable
  -nn cpu               Run noisy neighbor (stress-ng) on cpu id, for example, -nn 3
~~~

Note: The dump file is used to create the histogram. The file format is implicitly defined;
do not change it manually. If the `-dump-file` option is unset, the sample uses the 
`TCC_MEASUREMENTS_DUMP_FILE` environment variable. If the `TCC_MEASUREMENTS_DUMP_FILE`
environment variable is unset, a local file is created with the name `<application name>.hist`.

#### Example Command

~~~
tcc_measurement_analysis_sample hist "Multiplication:1000" -time-units ns -- tcc_multiple_measurements_sample --approximation 10 --multiplication 1000 --iterations 1000
~~~

#### Example Output

The sample prints the output of the profiled application and the profiling results.

The profiling output includes the following:

- Histogram name ("Multiplication" histogram)
- The table representing:
    + Latency Ranges: Ranges of recorded execution times
    + Count: Number of iterations that fall into each latency range
    + Histogram: Visual representation of the corresponding count
- Total counts: Total number of iterations
- Average execution time of the iterations
- Deviation from the average

~~~
APPLICATION OUTPUT:

Running with arguments:
    approximation = 10,
    multiplication = 1000,
    iterations = 1000
Running workloads. This may take a while, depending on iteration values.
Workloads were run successfully.

------------------------------
PROFILING OUTPUT:


-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
"Multiplication" histogram

Latency Ranges [ns]   Count   Histogram
(15, 16)    743   |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
(16, 17)    255   |||||||||||||||||||||||||
(17, 18)      0
(18, 19)      0
(19, 20)      0
(20, 21)      0
(21, 22)      1   |
(22, 23)      0
(23, 24)      0
(24, 25)      0
(25, 26)      0
(26, 27)      0
(27, 28)      0
(28, 29)      0
(29, 30)      0
(30, 31)      0
(31, 32)      0
(32, 33)      0
(33, 34)      0
(34, 35)      0
(35, 36)      0
(36, 37)      0
(37, 38)      0
(38, 39)      0
(39, 40)      0
(40, 41)      1   |

 Total counts: 1000
 Avg. Value: 16
 Std. Deviation: 1
~~~

### Create a Histogram for Single Measurement Sample

The following example creates a histogram for the single measurement sample.

#### Example Command

~~~
tcc_measurement_analysis_sample hist "Approximation:1000" -time-units ns -- tcc_single_measurement_sample --approximation 1000 --iterations 1000
~~~

#### Example Output

~~~
APPLICATION OUTPUT:

Running with arguments:
    approximation = 1000,
    iterations = 1000,
    outliers = False,
    deadline = N/A,
Approximation #1000 is:0.636620
[Approximation] Iterations 1000; iteration duration [ns]: avg=11257.000 min=11131.000 max=97494.000 jitter=86362.000

------------------------------
PROFILING OUTPUT:


-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
"Approximation" histogram

Latency Ranges [ns]   Count   Histogram
(11131, 13749)    998   |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
(13749, 16367)      0
(16367, 18985)      1   |
(18985, 21603)      0
(21603, 24221)      0
(24221, 26839)      0
(26839, 29457)      0
(29457, 32075)      0
(32075, 34693)      0
(34693, 37311)      0
(37311, 39929)      0
(39929, 42547)      0
(42547, 45165)      0
(45165, 47783)      0
(47783, 50401)      0
(50401, 53019)      0
(53019, 55637)      0
(55637, 58255)      0
(58255, 60873)      0
(60873, 63491)      0
(63491, 66109)      0
(66109, 68727)      0
(68727, 71345)      0
(71345, 73963)      0
(73963, 76581)      0
(76581, 79199)      0
(79199, 81817)      0
(81817, 84435)      0
(84435, 87053)      0
(87053, 89671)      0
(89671, 92289)      0
(92289, 94907)      0
(94907, 97525)      1   |

 Total counts: 1000
 Avg. Value: 11257
 Std. Deviation: 2742


### Create a Histogram for Multiple Measurements Sample

The following example creates a histogram for the multiple measurements sample.

#### Example Command

~~~
tcc_measurement_analysis_sample hist "Multiplication:1000,Approximation:1000,Cycle:1000" -time-units ns -- tcc_multiple_measurements_sample --approximation 100 --multiplication 100 --iterations 1000
~~~

#### Example Output

~~~
APPLICATION OUTPUT:

Running with arguments:
    approximation = 100,
    multiplication = 100,
    iterations = 1000
Running workloads. This may take a while, depending on iteration values.
Workloads were run successfully.

------------------------------
PROFILING OUTPUT:


-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
"Cycle" histogram

Latency Ranges [ns]   Count   Histogram
(4111867, 4115491)     53   ||||
(4115491, 4119115)    238   ||||||||||||||||||||||
(4119115, 4122739)    358   |||||||||||||||||||||||||||||||||
(4122739, 4126363)    245   ||||||||||||||||||||||
(4126363, 4129987)     70   ||||||
(4129987, 4133611)      9   |
(4133611, 4137235)      4   |
(4137235, 4140859)      1   |
(4140859, 4144483)      1   |
(4144483, 4148107)      0
(4148107, 4151731)      0
(4151731, 4155355)      1   |
(4155355, 4158979)      2   |
(4158979, 4162603)      1   |
(4162603, 4166227)      2   |
(4166227, 4169851)      2   |
(4169851, 4173475)      3   |
(4173475, 4177099)      0
(4177099, 4180723)      1   |
(4180723, 4184347)      0
(4184347, 4187971)      2   |
(4187971, 4191595)      1   |
(4191595, 4195219)      0
(4195219, 4198843)      1   |
(4198843, 4202467)      2   |
(4202467, 4206091)      0
(4206091, 4209715)      1   |
(4209715, 4213339)      1   |
(4213339, 4216963)      0
(4216963, 4220587)      0
(4220587, 4224211)      0
(4224211, 4227835)      0
(4227835, 4231459)      1   |

 Total counts: 1000
 Avg. Value: 4122564
 Std. Deviation: 9859
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
"Approximation" histogram

Latency Ranges [ns]   Count   Histogram
(1158, 1206)    957   |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
(1206, 1254)     10   |
(1254, 1302)     18   |
(1302, 1350)      7   |
(1350, 1398)      3   |
(1398, 1446)      2   |
(1446, 1494)      1   |
(1494, 1542)      0
(1542, 1590)      0
(1590, 1638)      0
(1638, 1686)      0
(1686, 1734)      0
(1734, 1782)      0
(1782, 1830)      0
(1830, 1878)      0
(1878, 1926)      0
(1926, 1974)      0
(1974, 2022)      0
(2022, 2070)      0
(2070, 2118)      1   |
(2118, 2166)      0
(2166, 2214)      0
(2214, 2262)      0
(2262, 2310)      0
(2310, 2358)      0
(2358, 2406)      0
(2406, 2454)      0
(2454, 2502)      0
(2502, 2550)      0
(2550, 2598)      0
(2598, 2646)      0
(2646, 2694)      0
(2694, 2742)      1   |

 Total counts: 1000
 Avg. Value: 1184
 Std. Deviation: 63
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
"Multiplication" histogram

Latency Ranges [ns]   Count   Histogram
(4110564, 4114167)     53   ||||
(4114167, 4117770)    234   |||||||||||||||||||||
(4117770, 4121373)    354   |||||||||||||||||||||||||||||||||
(4121373, 4124976)    250   |||||||||||||||||||||||
(4124976, 4128579)     73   ||||||
(4128579, 4132182)      9   |
(4132182, 4135785)      4   |
(4135785, 4139388)      1   |
(4139388, 4142991)      1   |
(4142991, 4146594)      0
(4146594, 4150197)      1   |
(4150197, 4153800)      0
(4153800, 4157403)      2   |
(4157403, 4161006)      1   |
(4161006, 4164609)      2   |
(4164609, 4168212)      2   |
(4168212, 4171815)      3   |
(4171815, 4175418)      0
(4175418, 4179021)      1   |
(4179021, 4182624)      0
(4182624, 4186227)      2   |
(4186227, 4189830)      1   |
(4189830, 4193433)      0
(4193433, 4197036)      1   |
(4197036, 4200639)      2   |
(4200639, 4204242)      0
(4204242, 4207845)      1   |
(4207845, 4211448)      1   |
(4211448, 4215051)      0
(4215051, 4218654)      0
(4218654, 4222257)      0
(4222257, 4225860)      0
(4225860, 4229463)      1   |

 Total counts: 1000
 Avg. Value: 4121249
 Std. Deviation: 9808
~~~

### Monitor Measurements

You can monitor latency measurements based on data collected by the measurement library during application run.

#### Command-Line Usage

~~~
usage: tcc_measurement_analysis_sample monitor [<measurement_instances>] [-deadline-only] [-dump-file] [-time-units] [-nn cpu] -- <app> [<args>]

command arguments:
  measurement_instances
                        Specify the measurement_instances inside the profiled application to use to collect the measurement history.
                        This argument can be omitted if environment variable TCC_MEASUREMENTS_BUFFERS is specified
                        (TCC_MEASUREMENTS_BUFFERS serves the same purpose). This argument overrides TCC_MEASUREMENTS_BUFFERS
                        environment variable.
                        Format: <measurement_instance_name>:<buffer_size>[,<measurement_instance_name>:<buffer_size> ...],
                        where the measurement instances name is the measurement name passed to __itt_string_handle_create()
                        in the profiled application, and the buffer size is the maximum number of measurements that the buffer can store.
                        Specifying too high a value will require lots of memory for the profiled application.
                        Example: "Cycle:1000,Sensor:1000"
                        NOTE: If the buffer size defined in the measurement_instances argument does not match the total number of
                        measurements (that is, only N last samples are collected), the full data statistics
                        (average, maximum, and minimum) may not match the histogram statistics.
  app                   Specify the path to the profiled application (example: ./tcc_multiple_measurements_sample).
  args                  Command-line arguments for the application being profiled.

options:
  -deadline-only        Print only the values exceeding the deadline.
  -h, --help            Show this help message and exit
  -dump-file file       Specify the history dump file path from the profiled application.
                        The file contains measurements collected by the measurement library during application run.
                        This option overrides TCC_MEASUREMENTS_DUMP_FILE environment variable
  -time-units units     Specify time units
                        "ns" for nanoseconds
                        "us" for microseconds
                        "clk" for CPU clock cycles (default)
                        This option overrides TCC_MEASUREMENTS_TIME_UNIT environment variable
  -nn cpu               Run noisy neighbor (stress-ng) on cpu id, for example, -nn 3
~~~

#### Example Command
~~~
tcc_measurement_analysis_sample monitor "Multiplication:10" -time-units ns -- tcc_multiple_measurements_sample --approximation 10 --multiplication 1000 --iterations 5
~~~

#### Output

~~~

MONITORING OUTPUT:


----------------------------------
Name                 Latency(ns)
----------------------------------
Multiplication       94
Multiplication       15
Multiplication       17
Multiplication       17
Multiplication       16

[Multiplication] Count of read data: 5

------------------------------

APPLICATION OUTPUT:

Running with arguments:
    approximation = 10,
    multiplication = 1000,
    iterations = 5
Running workloads. This may take a while, depending on iteration values.
Workloads were run successfully.
~~~

### Monitor Measurements and Deadline Violations

You can monitor latency measurements and deadline violations based on data collected by the measurement library during application run.

#### Example Command

~~~
tcc_measurement_analysis_sample monitor "Multiplication:10:20" -time-units ns -- tcc_multiple_measurements_sample --approximation 10 --multiplication 1000 --iterations 5
~~~

#### Example Output

~~~

MONITORING OUTPUT:

Monitoring "Multiplication" with deadline 19 ns.

----------------------------------
Name                 Latency(ns)
----------------------------------
Multiplication       33 Latency exceeding deadline
Multiplication       21 Latency exceeding deadline
Multiplication       16
Multiplication       16
Multiplication       17

[Multiplication] Count of read data: 5 (deadline violations: 2)

------------------------------

APPLICATION OUTPUT:

Running with arguments:
    approximation = 10,
    multiplication = 1000,
    iterations = 5
Running workloads. This may take a while, depending on iteration values.
Workloads were run successfully.
~~~

### Monitor Deadline Violations Only

You can monitor latency measurements and print only the deadline violations.

#### Example Command

```
tcc_measurement_analysis_sample monitor "Multiplication:10:20" -deadline-only -time-units ns -- tcc_multiple_measurements_sample --approximation 10 --multiplication 1000 --iterations 5
```

#### Example Output

```

MONITORING OUTPUT:

Monitoring "Multiplication" with deadline 19 ns.

----------------------------------
Name                 Latency(ns)
----------------------------------
Multiplication       40 Latency exceeding deadline

[Multiplication] Count of read data: 5  (deadline violations: 1)

------------------------------

APPLICATION OUTPUT:

Running with arguments:
    approximation = 10,
    multiplication = 1000,
    iterations = 5
Running workloads. This may take a while, depending on iteration values.
Workloads were run successfully.
```

### Monitor Single Measurement Instance

#### Example Command

~~~
tcc_measurement_analysis_sample monitor "Approximation:5" -time-units ns -- tcc_single_measurement_sample --approximation 100 --iterations 5
~~~

#### Example Output

~~~
MONITORING OUTPUT:


---------------------------------
Name                Latency(ns)
---------------------------------
Approximation       1827
Approximation       1373
Approximation       1160
Approximation       1158
Approximation       1160

[Approximation] Count of read data: 5

------------------------------

APPLICATION OUTPUT:

Running with arguments:
    approximation = 100,
    iterations = 5,
    outliers = False,
    deadline = N/A,
Approximation #100 is:0.636620
[Approximation] Iterations 5; iteration duration [ns]: avg=1335.000 min=1158.000 max=1827.000 jitter=669.000
~~~

### Monitor Multiple Measurement Instances

If your application has multiple measurement instances, you can monitor all of them in one report.
In the monitor command, separate the measurement instances with a comma, for example:
`Measurer1:10:2000,Measurer2:10:2000,Measurer3:10:2000`.

#### Example Command

```
tcc_measurement_analysis_sample monitor "Multiplication:5,Approximation:5,Cycle:5" -time-units ns -- tcc_multiple_measurements_sample --approximation 100 --multiplication 100 --iterations 5
```

#### Example Output

```
MONITORING OUTPUT:


----------------------------------
Name                 Latency(ns)
----------------------------------
Multiplication       4179995
Approximation        1766
Cycle                4182341
Approximation        1204
Cycle                4259842
Multiplication       4258136
Multiplication       4145680
Approximation        1302
Cycle                4147398
Approximation        1179
Cycle                4153221
Multiplication       4151673
Cycle                4164762
Multiplication       4163139
Approximation        1176

[Multiplication] Count of read data: 5
[Approximation] Count of read data: 5
[Cycle] Count of read data: 5

------------------------------

APPLICATION OUTPUT:

Running with arguments:
    approximation = 100,
    multiplication = 100,
    iterations = 5
Running workloads. This may take a while, depending on iteration values.
Workloads were run successfully.
```

## Command-Line Options

```
usage: tcc_measurement_analysis_sample hist | monitor

commands:
  {hist,monitor}  Supported commands
    hist          Show histogram from data collected by measurement library
    monitor       Monitor measurements collected by measurement library

options:
  -h, --help      Show this help message and exit

See 'tcc_measurement_analysis_sample <command> --help(-h)'
to get information about specific command.
```

Command-line options for `hist` command:
```
usage: tcc_measurement_analysis_sample hist [<measurement_instances>] [-dump-file] [-time-units] [-nn cpu] -- <app> [<args>]

command arguments:
  measurement_instances
                        Specify the measurement_instances inside the profiled application to use to collect the measurement history.
                        This argument can be omitted if environment variable TCC_MEASUREMENTS_BUFFERS is specified
                        (TCC_MEASUREMENTS_BUFFERS serves the same purpose). This argument overrides TCC_MEASUREMENTS_BUFFERS
                        environment variable.
                        Format: <measurement_instance_name>:<buffer_size>[,<measurement_instance_name>:<buffer_size> ...],
                        where the measurement instances name is the measurement name passed to __itt_string_handle_create()
                        in the profiled application, and the buffer size is the maximum number of measurements that the buffer can store.
                        Specifying too high a value will require lots of memory for the profiled application.
                        Example: "Cycle:1000,Sensor:1000"
                        NOTE: If the buffer size defined in the measurement_instances argument does not match the total number of
                        measurements (that is, only N last samples are collected), the full data statistics
                        (average, maximum, and minimum) may not match the histogram statistics.
  app                   Specify the path to the profiled application (example: ./tcc_multiple_measurements_sample).
  args                  Command-line arguments for the application being profiled.

options:
  -h, --help            Show this help message and exit
  -dump-file file       Specify the history dump file path from the profiled application.
                        The file contains measurements collected by the measurement library during application run.
                        This option overrides TCC_MEASUREMENTS_DUMP_FILE environment variable
  -time-units units     Specify time units
                        "ns" for nanoseconds
                        "us" for microseconds
                        "clk" for CPU clock cycles (default)
                        This option overrides TCC_MEASUREMENTS_TIME_UNIT environment variable
  -nn cpu               Run noisy neighbor (stress-ng) on cpu id, for example, -nn 3

```

Command-line options for `monitor` command:
```
usage: tcc_measurement_analysis_sample monitor [<measurement_instances>] [-deadline-only] [-dump-file] [-time-units] [-nn cpu] -- <app> [<args>]

command arguments:
  measurement_instances
                        Specify the measurement_instances inside the profiled application to use to collect the measurement history.
                        This argument can be omitted if environment variable TCC_MEASUREMENTS_BUFFERS is specified
                        (TCC_MEASUREMENTS_BUFFERS serves the same purpose). This argument overrides TCC_MEASUREMENTS_BUFFERS
                        environment variable.
                        Format: <measurement_instance_name>:<buffer_size>[,<measurement_instance_name>:<buffer_size> ...],
                        where the measurement instances name is the measurement name passed to __itt_string_handle_create()
                        in the profiled application, and the buffer size is the maximum number of measurements that the buffer can store.
                        Specifying too high a value will require lots of memory for the profiled application.
                        Example: "Cycle:1000,Sensor:1000"
                        NOTE: If the buffer size defined in the measurement_instances argument does not match the total number of
                        measurements (that is, only N last samples are collected), the full data statistics
                        (average, maximum, and minimum) may not match the histogram statistics.
  app                   Specify the path to the profiled application (example: ./tcc_multiple_measurements_sample).
  args                  Command-line arguments for the application being profiled.

options:
  -deadline-only        Print only the values exceeding the deadline.
  -h, --help            Show this help message and exit
  -dump-file file       Specify the history dump file path from the profiled application.
                        The file contains measurements collected by the measurement library during application run.
                        This option overrides TCC_MEASUREMENTS_DUMP_FILE environment variable
  -time-units units     Specify time units
                        "ns" for nanoseconds
                        "us" for microseconds
                        "clk" for CPU clock cycles (default)
                        This option overrides TCC_MEASUREMENTS_TIME_UNIT environment variable
  -nn cpu               Run noisy neighbor (stress-ng) on cpu id, for example, -nn 3

```

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
