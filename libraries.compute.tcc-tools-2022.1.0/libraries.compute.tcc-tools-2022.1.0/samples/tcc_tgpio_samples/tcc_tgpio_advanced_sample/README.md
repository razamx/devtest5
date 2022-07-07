# tcc_tgpio_advanced_sample
This sample is a self-contained example program demonstrating 
output scenarios with software GPIO and Time-Aware GPIO (TGPIO).

## Source Files

 File | Description
 ---- | -----------
 [main.c](src/main.c)| Main file containing TGPIO sample
 [tgpio_sample.h](src/tgpio_sample.h)| TGPIO sample header
 [swgpio.c](../../common/src/swgpio.c)| Software GPIO service functions
 [swgpio.h](../../common/include/swgpio.h)| Software GPIO service functions
 [tgpio.c](../../common/src/tgpio.c)| TGPIO service functions
 [tgpio.h](../../common/include/tgpio.h)| TGPIO service functions
 [time_convert.c](../../common/src/time_convert.c)| Time convert service functions
 [time_convert.h](../../common/include/time_convert.h)| Time convert service functions
 [tcc_signal_analyzer_plotter.py](../../plot_scripts/tcc_signal_analyzer_plotter.py)| Data analysis script

## Run the Sample Output Scenario
```
tcc_tgpio_advanced_sample --mode tgpio --signal_file signal.txt
```

The software GPIO signal file has the following parameters:
* pin: Pin for output
* offset: Offset since start_time, nanoseconds
* period: Output period for this pin, nanoseconds

Signal file format:
```
<pin> <offset> <period>
<pin> <offset> <period>
```

Example: To generate two square waves with a period of 1 second and a half-period phase shift on pins 1 and 2, describe each pin only once:
```
1 000000000 1000000000
2 500000000 1000000000
```

The TGPIO signal file has the following parameters:
* pin: Pin for output
* offset: Offset since start_time, nanoseconds
* period: Output period for this pin, nanoseconds
* channel: Channel for pin
* device: TGPIO device

Signal file format:
```
<pin> <offset> <period> <channel> <device>
<pin> <offset> <period> <channel> <device>
```

Example: To generate two square waves with a period of 1 second and a half-period phase shift on pins 0 and 1 from device /dev/ptp0, describe each pin only once:
```
0 000000000 1000000000 0 /dev/ptp0
1 500000000 1000000000 1 /dev/ptp0
```

## Command-Line Options
### Sample command-line options

```
Output scenario:
tcc_tgpio_advanced_sample --mode <mode> --signal_file <path>

Options:
    -m, --mode         Specify the mode (soft or tgpio) to run.
    -c, --signal_file  Specify the path to the signal config file. Required for output mode.
```

### Script command-line options

```
usage: tcc_signal_analyzer_plotter.py [-h] (--period | --shift) [--output OUTPUT]
                                      [--min MIN] [--max MAX] [--units {s,ms,us,ns}]
                                      log_file [log_file ...]

Analyzes the captured data from tcc_tgpio_advanced_sample. Calculates jitter, other statistics, and plot distribution

positional arguments:
  log_file              Log file or files

optional arguments:
  -h, --help            Show this help message and exit
  --period              Plot the periodic signal
  --shift               Plot the phase difference between two signals
  --output OUTPUT       Output file; otherwise, display in window
  --min MIN             Minimum plotting bound
  --max MAX             Maximum plotting bound
  --units {s,ms,us,ns}  Time units to display (default: nanosecond)
```

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
 

