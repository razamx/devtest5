
## Getting Started with Intel® TCC Tools

* [Complete the Installation of the setup](Installation.md)
* [Run Sample Workloads](#run-sample-workloads)
* [Logging support](#logging-support)

## Run sample workloads
The workload used here is **tcc_sensor_data_sample**

More information is at [sensor_data_sample](../samples/sensor/README.md)
This will guide you through using the Intel® TCC Tools, detailing it's usage, APIs,
source code and examples.

### How to run the samples

To test the installation run the following commands:

#### Ensure the resctrl filesystem is mounted

Check if this file exists `/sys/fs/resctrl/schemata`
If it does not exist then Pseudo-Locking won't be supported.

Please mount resctrl
~~~
mount -t resctrl resctrl /sys/fs/resctrl
~~~

#### Sensor Sample without Pseudo-Locking
~~~
root@intel-corei7-64:~# tcc_hotspot find tcc_sensor_data_sample \
-args "-m /opt/intel/tccsdk/share/tcc/sensor_data_sample/model/accel_udriver_lr.txt \
-f /opt/intel/tccsdk/share/tcc/sensor_data_sample/data/NotOnTable_1511809436.accel.test.csv \
-s 0.1"
~~~

Expected Output:
~~~
sensor filename: /opt/intel/tccsdk/share/tcc/sensor_data_sample/data/NotOnTable_1511809436.accel.test.csv
model filename: /opt/intel/tccsdk/share/tcc/sensor_data_sample/model/accel_udriver_lr.txt
use cache lock: 0
cpuid: 3
verbose: 0
measure: 0
sampling factor: 0.100000
window size in seconds: 60
Samples Processed: 8957 State 0: 0 State 1: 5957
[Cycle] Iterations 5957; iteration duration: avg=2463051.997 min=2340390.000 max=5527132.000 jitter=3186742.000
[Sensor] Iterations 5957; iteration duration: avg=77671.154 min=20160.000 max=294116.000 jitter=273956.000
[Model] Iterations 5957; iteration duration: avg=2384201.006 min=2259242.000 max=5349010.000 jitter=3089768.000
[Scheduling] Iterations 5957; iteration duration: avg=3543347.000 min=3142822.000 max=3784212.000 jitter=641390.000
tcc_hotspot: tcc_sensor_data_sample hotspot is root_mean_square 11.62%
~~~

#### Sensor Sample with Pseudo-Locking

~~~
root@intel-corei7-64:~/tccsdk# tcc_hotspot find tcc_sensor_data_sample \
-args "-m /opt/intel/tccsdk/share/tcc/sensor_data_sample/model/accel_udriver_lr.txt \
-f /opt/intel/tccsdk/share/tcc/sensor_data_sample/data/NotOnTable_1511809436.accel.test.csv \
-s 0.1 -l"
~~~

Expected Output:
~~~
sensor filename: /opt/intel/tccsdk/share/tcc/sensor_data_sample/data/NotOnTable_1511809436.accel.test.csv
model filename: /opt/intel/tccsdk/share/tcc/sensor_data_sample/model/accel_udriver_lr.txt
use cache lock: 1
cpuid: 3
verbose: 0
measure: 0
sampling factor: 0.100000
window size in seconds: 60
Samples Processed: 8957 State 0: 0 State 1: 5957
[Cycle] Iterations 5957; iteration duration: avg=2343084.313 min=2288832.000 max=3478758.000 jitter=1189926.000
[Sensor] Iterations 5957; iteration duration: avg=21577.642 min=13418.000 max=742910.000 jitter=729492.000
[Model] Iterations 5957; iteration duration: avg=2320876.653 min=2266580.000 max=3454668.000 jitter=1188088.000
[Scheduling] Iterations 5957; iteration duration: avg=3128610.725 min=3116846.000 max=3648240.000 jitter=531394.000
tcc_hotspot: tcc_sensor_data_sample hotspot is 0x0000000000026b30 1.22%
~~~

Since the hotspot root_mean_square is pseudo locked to the cache using
the tcc_cache_lock API. `-l` in the `args` parameter above, ensures that the function is locked.

## Logging support

Intel® TCC Tools uses logging internally. To enable log output you can set TCC_LOG_LEVEL environment variable.

 Possible values |
-----------------|
 TRACE |
 DEBUG |
 INFO |
 WARNING |
 ERROR |
 FATAL |
 NONE |

 To redirect log output to file you can set TCC_LOG_TO_FILE environment variable.
 ~~~
 TCC_LOG_TO_FILE=somefile.log TCC_LOG_LEVEL=DEBUG ./sample
 ~~~
