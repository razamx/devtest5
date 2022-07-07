# tcc_tgpio_frequency_discipline_sample

This sample is a simple self-contained example program that generates a signal as close to the required frequency as possible (with the smallest period and phase errors) by changing Time-Aware GPIO (TGPIO) period repeatedly. The generated signal frequency is limited by the frequency of the Always Running Timer (ART) clock. Therefore, the real signal frequency may differ from the expected signal frequency. The sample measures the real signal period and uses a proportional integral (PI) controller to change the period to achieve the closest possible average frequency.

**Note:** Samples are intended to demonstrate how to write code using certain features. They are not performance benchmarks. Example output shown here is for illustration only. Your output may vary.

## Source Files

 File | Description
 ---- | -----------
 [main.c](src/main.c)| Main file containing sample
 [tgpio.c](../../common/src/tgpio.c)| TGPIO service functions
 [tgpio.h](../../common/include/tgpio.h)| TGPIO service functions
 [time_convert.c](../../common/src/time_convert.c)| Time convert service functions
 [time_convert.h](../../common/include/time_convert.h)| Time convert service functions
 [frequency_discipline_sample_analyzer.py](../../plot_scripts/frequency_discipline_sample_analyzer.py)| Data analysis script

## Prerequisites 

Acquire a logic analyzer.

## Example Command

1. Connect the TGPIO pin 0 to channel 0 of the logic analyzer. Connect the ground pin to the ground pin of the logic analyzer (any channel).

2. Generate a signal with frequency discipline: 
   ```
   tcc_tgpio_frequency_discipline_sample --pin 0 --frequency 96000 --proportional 1 --integral 0.00001
   ```

3. In the logic analyzer software, start data capture on channel 0 for 15 seconds.

4. Wait until the logic analyzer stops capturing data.

5. Stop the sample (Ctrl+C).

6. Export the data from the logic analyzer software as `discipline.csv`.

### Plot the Data

```
frequency_discipline_sample_analyzer.py discipline.csv --frequency 96000 
```

## Command-Line Options
### Sample Command-Line Options
```
Usage:
tcc_tgpio_frequency_discipline_sample [--pin N] [--channel N] [--device <device_name>] [--period N | --frequency N] [--sleep N] [--proportional N] [--integral N]
Options:
    --pin               Specify the output pin index. Default: 0.
    --channel           Specify the channel for the output pin. Default: 0.
    --device            Specify the TGPIO device. Default: /dev/ptp0.
    --period            Specify the output period in nanoseconds. Default: 100000.
    --frequency         Specify the output frequency in hertz. Default: 10000.
    --sleep             Specify the interval in nanoseconds between changing the output period. Default: 100000000.
    --proportional      Specify the proportional gain of the PI controller. Default: 0.
    --integral          Specify the integral gain of the PI controller. Default: 0.

```

### Script Command-Line Options
```
usage: frequency_discipline_sample_analyzer.py [-h]
                                               (--period PERIOD | --frequency FREQUENCY)
                                               [--units {s,ms,us,ns,Hz,kHz,MHz}]
                                               files [files ...]

Analyzes the captured data from tcc_tgpio_frequency_discipline_sample

positional arguments:
  files                 Log file or files

optional arguments:
  -h, --help            Show this help message and exit
  --period PERIOD       Expected period
  --frequency FREQUENCY
                        Expected frequency
  --units {s,ms,us,ns,Hz,kHz,MHz}
                        Time units to display
```

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
 
