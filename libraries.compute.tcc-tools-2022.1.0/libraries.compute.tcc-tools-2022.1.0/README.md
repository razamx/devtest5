# Intel® Time Coordinated Computing (Intel® TCC)

Intel® TCC Tools is a set of APIs, tools, and code samples enabling
developers to take advantage of Intel® Time Coordinated Computing
(Intel® TCC) related hardware features on Intel platforms. Intel® TCC
features enable developers to increase determinism and reduce latencies
for resource access (such as memory) as well as sync events across different
systems through highly accurate timestamps. Targeting real-time
workloads, Intel® TCC features provide mechanisms to reduce jitter and reduce
overall worst-case execution time.

_Intel® Internal Evaluation Only_

| [API](#api-reference) | [Tools](#tools) | [Samples](#samples) | [Supported Systems](#supported-systems)

## Features

### API Reference

API reference documentation in Doxygen format is located in /usr/share/tcc_tools/doc/api/index.html in the Yocto image.

API Type | Description                                               | Purpose
---------|-----------------------------------------------------------|--------------------------------------------------------------
[cache](./include/tcc/cache.h)        | Cache allocation to meet latency requirements   | Provides ability to allocate memory for critical data structures in cache.
[measurement](./include/tcc/measurement.h)  | Low-overhead time measurements based on Timestamp Counter | Provides ability to accurately measure the average, max, min latencies for user-defined periods.

### Tools

 Tool | Description
 ---- | -----------
 [tcc_rt_checker](./tools/rt_checker) | Detects whether dependent features are available and/or configured.
 [tcc_cache_configurator](./tools/cache_configurator)  | Provides mechanisms for user to configure, discover, and manage cache memory resources.
 [tcc_data_streams_optimizer](./tools/tcc_data_streams_optimizer) | Optimizes transfer of data between two endpoints on the platform.

### Samples

 Sample                             | Description
 ---------------------------------- | -----------
 [tcc_cache_allocation_sample](./samples/tcc_cache_allocation_sample)   | Simple example demonstrating cache allocation library
 [tcc_measurement_monitoring_sample](./samples/tcc_measurement_monitoring_sample) | Simple self-contained example of a monitoring application
 [tcc_measurement_analysis_sample](./samples/tcc_measurement_analysis_sample) | Simple example of a profiling application
 [tcc_rt_communication_demo](./samples/tcc_rt_communication_demo) | Exercise a combination of Time-Sensitive Networking (TSN) and cache allocation

## Supported Systems

* Intel Atom® x6000E Series processors (code name: Elkhart Lake)
* 11th Gen Intel® Core™ processors (code name: Tiger Lake UP3)
