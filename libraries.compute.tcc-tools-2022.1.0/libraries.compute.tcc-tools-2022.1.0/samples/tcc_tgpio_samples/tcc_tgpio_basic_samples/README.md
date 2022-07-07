# Basic TGPIO Samples

The basic samples demonstrate how to start working with Time-Aware GPIO (TGPIO):
- [tcc_tgpio_info](#tcc_tgpio_info)
- [tcc_tgpio_basic_input](#tcc_tgpio_basic_input)
- [tcc_tgpio_basic_periodic_output](#tcc_tgpio_basic_periodic_output)
- [tcc_tgpio_basic_oneshot_output](#tcc_tgpio_basic_oneshot_output)

**Note:** Samples are intended to demonstrate how to write code using certain 
features. They are not performance benchmarks. Example output shown here is for 
illustration only. Your output may vary.



## tcc_tgpio_info
This sample is a simple self-contained example program demonstrating how to get
a report of available Time-Aware GPIO (TGPIO) capabilities for a specified 
device.

### Source Files

 File | Description
 ---- | -----------
 [tgpio_info.c](src/tgpio_info.c)| Main file containing TGPIO sample.


### Example Command
```
tcc_tgpio_info
```

### Example Output
```
Available capabilities:
  Pins: 2
  Input function  - External timestamp channels: 2
  Output function - Programmable periodic signals: 2
  Precise system-device cross-timestamps: Supported
=======================================================
Available TGPIO pins:
| Pin Name              | Index | Used Function          | Channel |
--------------------------------------------------------------------
| intel-pmc-tgpio-pin01 | 0     | None                   | 0       |
--------------------------------------------------------------------
| intel-pmc-tgpio-pin02 | 1     | None                   | 1       |
```

### Command-Line Options
```
Usage: tcc_tgpio_info [--device <device_name>]
Options:
    --device   Specify the TGPIO device. Default: "/dev/ptp0".
```

## tcc_tgpio_basic_input
This sample is a simple self-contained example program demonstrating a Time-Aware 
GPIO (TGPIO) input scenario. The sample prints a timestamp when an edge is detected,
indicating a change in the TGPIO input pin state.

### Source Files

 File | Description
 ---- | -----------
 [tgpio_basic_input.c](src/tgpio_basic_input.c)| Main file containing TGPIO sample.

### Command-Line Options
```
Usage: tcc_tgpio_basic_input [--pin N] [--channel N] [--device <device_name>]
Options:
    --pin      Specify the input pin index. Default: 0.
    --channel  Specify the channel for the input pin. Default: 0.
    --device   Specify the TGPIO device. Default: "/dev/ptp0".
```

### Example Command

1. Connect the TGPIO input pin to the GND pin with a wire. This will set the logic to “0” on the input pin.

2. Run the sample:
```
tcc_tgpio_basic_input
```

3. Disconnect the wire from the GND pin, and connect it to the VCC pin. This will set the logic to “1” on the input pin.

### Example Output
```
Start input sample.
Pin 0, channel 0
To interrupt, use Ctrl+C
Detected edge on channel 0 at 19:37:43.836201899
```

## tcc_tgpio_basic_periodic_output
This sample is a simple self-contained example program demonstrating a Time-Aware
GPIO (TGPIO) periodic output scenario. The sample generates a signal based on
the specified period (1 second, for example).

### Source Files

 File | Description
 ---- | -----------
 [tgpio_basic_periodic_output.c](src/tgpio_basic_periodic_output.c)| Main file containing TGPIO sample.

### Example Command
This example shows how to connect the TGPIO pin to the software GPIO pin to check that TGPIO works.

1. Connect TGPIO pin 0 to TGPIO pin 1 with a wire.

2. Open two terminal windows.

3. In the first terminal window, run the sample: 
```
tcc_tgpio_basic_periodic_output
```

4. In the second terminal window, run the `tcc_tgpio_basic_input` to read the values
```
tcc_tgpio_basic_input --pin 1 --channel 1
```

### Example Output
From swgpio_basic_input.sh:
```
Start input sample.
Pin 1, channel 1
To interrupt, use Ctrl+C
Detected edge on channel 1 at 16:41:32.000000000
Detected edge on channel 1 at 16:41:32.500000000
Detected edge on channel 1 at 16:41:33.000000000
Detected edge on channel 1 at 16:41:33.500000000
Detected edge on channel 1 at 16:41:34.000000000
```

### Command-Line Options
#### Sample command-line options
```
Usage: tcc_tgpio_basic_periodic_output [--pin N] [--channel N] [--device <device_name>] [--period N]
Options:
    --pin      Specify the output pin index. Default: 0.
    --channel  Specify the channel for the output pin. Default: 0.
    --device   Specify the TGPIO device. Default: "/dev/ptp0".
    --period   Specify the output period in nanoseconds. Default: 1000000000 (1 second).
```

## tcc_tgpio_basic_oneshot_output
This sample is a simple self-contained example program demonstrating a Time-Aware
GPIO (TGPIO) single shot output scenario. The sample generates a single pulse after
a specified delay (1 second, for example).

### Source Files

 File | Description
 ---- | -----------
 [tgpio_basic_oneshot_output.c](src/tgpio_basic_oneshot_output.c)| Main file containing TGPIO sample.

### Example Command
This section explains how to connect the TGPIO pin to an input pin on a Logic Analyzer (LA) to check that TGPIO works.

1. Connect TGPIO pin 0 to LA channel 0

2. Open the LA software and start capturing on channel 0 or set a trigger on edge transitions

3. In a terminal window, run the sample: 
```
tcc_tgpio_basic_oneshot_output
```

### Example Output

```
          ________
_________|  

^ 1 sec  ^
```

Edge transitions seen in the output will be rising or falling based on the pin's initial state.

### Command-Line Options
#### Sample command-line options
```
Usage: tcc_tgpio_basic_oneshot_output [--pin N] [--channel N] [--device <device_name>] [--start N]
Options:
    --pin      Specify the output pin index. Default: 0.
    --channel  Specify the channel for the output pin. Default: 0.
    --device   Specify the TGPIO device. Default: "/dev/ptp0".
    --start    Specify the delay until the pulse starts. Default: 1000000000 (1 second).
```

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
 
