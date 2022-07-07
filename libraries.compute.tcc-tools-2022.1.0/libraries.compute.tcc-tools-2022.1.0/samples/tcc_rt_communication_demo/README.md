# tcc_rt_communication_demo

The tcc_rt_communication_demo application is built on top of the open62541 library to exercise a combination of
Time-Sensitive Networking (TSN) and Intel® Time Coordinated Computing (Intel® TCC) features.

The tcc_rt_communication_demo is a self-contained set of scripts and programs that demonstrates the benefit
of combining the cache pseudo-locking capability of the Buffer Allocation API with Time-Sensitive Networking (TSN) Reference
Software provided by Intel. The sample simulates a scenario where one system runs
a data processing workload, then sends a data packet to another system.

The demo supports optimized (opt) and non-optimized (noopt) modes of operation, here is an overview how the modes compare:
1) The data processing workload allocates the work buffer in the System RAM for the non-optimized mode, and in L2 Cache for the optimized mode.
2) TSN network communication is implemented using raw sockets (AF_PACKET) for non-optimized mode, and eXpress Data Path (XDP) sockets for the optimized mode.
3) The sample liverages the Ethernet Virtual Channels in the optimized mode.

`NОТЕ`: Samples are intended to demonstrate how to write code using certain features. They are not performance benchmarks. Example output shown here is for illustration only. Your output may vary.

## Source Code and Dependencies

On Yocto Project, the required dependencies are already installed.

### Source Files

 File | Description
 ---- | -----------
 [json_helper.c/h](src/opcua-pubsub-server/json_helper.c)| Auxiliary files with helper JSON functions and definitions.
 [multicallback_server.c](src/opcua-pubsub-server/multicallback_server.c)| Main source file with startup and server configuration functions.
 [opcua_common.c/h](src/opcua-pubsub-server/opcua_common.c)| Auxiliary files with base helper functions and common data definitions.
 [opcua_publish.c/h](src/opcua-pubsub-server/opcua_publish.c)| Auxiliary files with OPC UA* publisher functions and definitions.
 [opcua_subscribe.c/h](src/opcua-pubsub-server/opcua_subscribe.c)| Auxiliary files with OPC UA* subscriber functions and definitions.
 [opcua_custom.c/h](src/opcua-pubsub-server/opcua_custom.c)| Auxiliary files with OPC UA* threading and timer related functions and definitions.
 [opcua_datasource.c/h](src/opcua-pubsub-server/opcua_datasource.c)| Auxiliary files with OPC UA* data producing/consuming callback functions.
 [opcua_shared.h](src/opcua-pubsub-server/opcua_shared.h)| Auxiliary files with structures definitions shared between OPC UA* server and data manager.
 [opcua_utils.h](src/opcua-pubsub-server/opcua_utils.h)| Auxiliary files with defines of utility API used for logging and debugging functionality.
 [compute.c](src/data-mgr/compute.c)| This file contains implementation of input data generation and output data producer.
 [listener.c](src/data-mgr/listener.c)| This file contains implementation of listener functionality monitor data in shared memory buffer.
 [talker.c](src/data-mgr/talker.c)| This file contains implementation of talker functionality to publish data to shared memory buffer.
 [pcheck.h](src/data-mgr/pcheck.h)| Auxiliary file with implementation of assertion macros.
 [pconfig.h](src/data-mgr/pconfig.h)| Auxiliary file with definitions of data configuration structures used by data manager.
 [pdatasource.c/h](src/data-mgr/pdatasource.c)| Auxiliary file with implementation of logic for data generation using Buffer API and pointer chasing workload.
 [ptimer.c/h](src/data-mgr/ptimer.c)| This file contains API implementation for a periodic real-time (RT) Timer.


## Prerequisites
To run the sample, you need two boards, which are equipped with a TSN compatible Intel® Ethernet Controller.
The boards must be connected to each other via `CAT-5E` or better (for example, `CAT-6`) Ethernet cable.
The boards must have the supported Yocto Project-based image installed.
See the [Get Started Guide](https://software.intel.com/en-us/tcc-tools-get-started-guide-0-10) for more
information about supported boards and image.

## Run the Sample using preinstalled scripts and binaries in image
`NOTE`: Start the sample on Board B first. The sample on Board B will start in listener mode and will wait for incoming packets. Then start the sample on Board A.

`NOTE`: The network interface name may vary from board to board. Pick a valid name from your system.

Executable tcc_rt_communication_demo will automatically start TSN time synchronization, configure transmission queues on Board A, and initiate transmission of
packets between Board A and Board B.

`NOTE`: TSN time synchronization may take about 30 seconds. Do not interrupt the sample during this period.

### Run the following commands for the basic example:
*In this example, a talker (Board A) sends messages to a listener (Board B), using the open62541 OPC UA* Publish-Subscribe APIs. This example uses the Integrated TSN Controller, IEEE 802.1AS, IEEE 802.1Qbv, and Time-based Packet Scheduling.*

| Board A                             | Board B
|-------------------------------------|------------------------------------------------------
|                                     | tcc_rt_communication_demo --profile basic-b-noopt --interface enp1s0
|tcc_rt_communication_demo --profile basic-a-noopt --interface enp1s0 |

Wait until the sample finishes on Board B. Output files are generated.

### Run the following commands for the SISO example:
*SISO stands for single input, single output. In this example, one platform (the monitor and actuator - Board A) transmits a packet to another platform (the compute - Board B) which returns a response to the first platform using the open62541 OPC UA* Publish-Subscribe APIs. This example uses the Integrated TSN Controller, IEEE 802.1AS, IEEE 802.1Qbv, and Time-based Packet Scheduling.*
For accurate results, you should launch the sample simultaneously on two boards within 2-3 sec interval.

| Board A                             | Board B
|-------------------------------------| ------------------------------------------------------
|                                     | tcc_rt_communication_demo --profile siso-single-b-noopt --interface enp1s0
|tcc_rt_communication_demo --profile siso-single-a-noopt --interface enp1s0 |

Wait until the sample finishes on Board A. Output files are generated.

### Run the SISO example from one board
The *SISO example requires the sample is launched on Board A and Board B simultaneously within 2-3 sec interval, and this might be difficult
to accomplish reliably. To faciliate this the remote launch mode is introduced.
In this mode, we can launch the *SISO example only on one board (Board A), the SISO example on Board B is launched automatically,
selecting the same start time (base time to start netowrk communication).
Currently, the remote launch mode can be used together with `--mode setup` argument for setting up the TSN network interfaces, the traffic control,
the time synchronization, and with `--mode run` argument for running the sample.
In order to switch to the remote launch mode, the optional argument `--remote-address <remote-target-IP>` should be
used. If the sample is switched to the remote launch mode, the following arguments can also be used:
`--remote-interface <remote-interface>` specifying the remote network interface name, `--remote-config-path <remote-config-path>`
specifies the path to the configuration files on the remote side, and `--remote-exec-path <remote-exec-path>` defining the path to the
sample executables on the remote board. Currently the sample on the remote side will be launched in a pre-defined working directory:
`/tmp`, the output directoty is also pre-defined as: `/tmp/tcc_rt_communication_demo_output`. The remote paths should be specified
with full paths. For the relative paths they should be specified relative to the working directory `/tmp`.
The passwordless ssh connection should be configured between the boards before the remote launch mode can be used. Run the following
command to setup the passwordless connection between the boards:
```
ssh-copy-id <board-B-IP-address>
```
After this, the sample can be launched from one side (Board A), here are example command lines for setting up and running the RTC sample:

| Board A                             | Board B
|-------------------------------------| ------------------------------------------------------
|tcc_rt_communication_demo --profile siso-single-a-noopt --interface enp1s0 --remote-address 192.168.1.100 --mode setup --remote-interface enp1s0 --remote-config-path /home/user/sample/cfg --remote-exec-path /home/user/sample/install |
|tcc_rt_communication_demo --profile siso-single-a-noopt --interface enp1s0 --remote-address 192.168.1.100 --mode run --remote-interface enp1s0 --remote-config-path /home/user/sample/cfg --remote-exec-path /home/user/sample/install |

## Build the Sample for running from Install folder
1. Go to the sample directory. On the Yocto Project-based image, run the following command:
```
cd /usr/share/tcc_tools/samples/tcc_rt_communication_demo
```

2. Build and install the sample:
```
make
make install
```

## Run the Sample from the Install folder
Build the sample as described above. Use the directory where you built the sample for `cd ` commands in the table below.

`NOTE`: Start the sample on Board B first. The sample on Board B will start in listener mode and will wait for incoming packets. Then start the sample on Board A.

`NOTE`: The network interface name may vary from board to board. Pick a valid name from your system.

Script tcc_rt_communication_demo.py will automatically start TSN time synchronization, configure transmission queues on Board A, and initiate transmission of
packets between Board A and Board B.

`NOTE`: TSN time synchronization may take about 30 seconds. Do not interrupt the sample during this period.

### Run the following commands for the basic example:
*In this example, a talker (Board A) sends messages to a listener (Board B), using the open62541 OPC UA* Publish-Subscribe APIs. This example uses the Integrated TSN Controller, IEEE 802.1AS, IEEE 802.1Qbv, and Time-based Packet Scheduling.*

| Board A                             | Board B
|-------------------------------------|------------------------------------------------------
|                                     | cd /usr/share/tcc_tools/samples/tcc_rt_communication_demo/install
|                                     | ./tcc_rt_communication_demo.py --profile basic-b-noopt --interface enp1s0 --exec-path "${PWD}"
|cd /usr/share/tcc_tools/samples/tcc_rt_communication_demo/install |
|./tcc_rt_communication_demo.py --profile basic-a-noopt --interface enp1s0 --exec-path "${PWD}"      |

Wait until the sample finishes on Board B. Output files are generated.

### Run the following commands for the SISO example:
*SISO stands for single input, single output. In this example, one platform (the monitor and actuator - Board A) transmits a packet to another platform (the compute - Board B) which returns a response to the first platform using the open62541 OPC UA* Publish-Subscribe APIs. This example uses the Integrated TSN Controller, IEEE 802.1AS, IEEE 802.1Qbv, and Time-based Packet Scheduling.*

| Board A                             | Board B
|-------------------------------------| ------------------------------------------------------
|                                     | cd /usr/share/tcc_tools/samples/tcc_rt_communication_demo/install
|                                     | ./tcc_rt_communication_demo.py --profile siso-single-b-noopt --interface enp1s0 --exec-path "${PWD}"
|cd /usr/share/tcc_tools/samples/tcc_rt_communication_demo/install   |
|./tcc_rt_communication_demo.py --profile siso-single-a-noopt --interface enp1s0 --exec-path "${PWD}" |

Wait until the sample finishes on Board A. Output files are generated.

### Modes

The sample provides a number of optional modes that may be useful in certain situations.

Option                               | Description | Example
------------------------------------ | ----------- | -----------------------------------------
run | Runs the application only.     | Use this mode when restarting the sample. In this case, resynchronization of the clock and TSN settings will not start.
clock | Starts or restarts the clock synchronization services (ptp4l and phc2sys). | Use this mode when the clock is out of sync and the plot shows too large or negative latency values.
tsn | Configures the network interface for TSN. | Use this mode after observing that the basic profile starts the SISO profile or vice versa. `NOTE`: Wait at least 30 seconds for the sample to configure TSN.
setup | Similar to the all mode, except that it does not start the demo application: (re-)configure the network interfaces, traffic control, and (re-)start ptp4l and phc2sys clock syncronization.

```
Example: ./tcc_rt_communication_demo.py --profile basic-a-noopt --interface enp1s0 --mode run
```

## Plot the Data from Log Files

On your host system, you can use `tcc_rt_communication_demo_plotter.py` to plot the data from
log-e2e.log (Board B) and log-roundtrip.log (Board A).

### Prerequisites:

On your host system, go to the script directory:
```
cd <toolchain installation directory>/sysroots/corei7-64-poky-linux/usr/share/tcc_tools/plot_scripts
```
Install required python modules:
```
sudo pip3 install -r prerequisites.txt
```
or
```
sudo python3 -m pip install -r prerequisites.txt
```

### To plot the data:

On your host system, go to the script directory:
```
cd <toolchain installation directory>/sysroots/corei7-64-poky-linux/usr/share/tcc_tools/plot_scripts
```
1. Copy tcc_rt_communication_demo_output/log-e2e.log and tcc_rt_communication_demo_output/log-roundtrip.log to the script directory.

2. Run the script.
```
python3 tcc_rt_communication_demo_plotter.py --input log-e2e.log --output e2e.png
python3 tcc_rt_communication_demo_plotter.py --input log-roundtrip.log --output roundtrip.png
```
3. Several log files can be passed to the script (for example for optimized and non-optimized profiles), so, the statistics will be shown on one diagram:
```
python3 tcc_rt_communication_demo_plotter.py --input log_e2e-opt.log log_e2e-noopt.log --output e2e.png
```
The script prints latency statistics to the console and generates images dependent on arguments.

To explore all script parameters:
```
python3 tcc_rt_communication_demo_plotter.py
```

### To plot the data in Log Files available on target
We can also use the `tcc_rt_communication_demo_plotter.py` script on the host system to plot diagrams for the Log Files avaialble on the target system directly.
For this we need to specify the target hostname (or IP address) passing the `--address <hostname>` argument. In this mode we can also specify the username
for accessing the target system passing `--user <username>` argument (`root` user is used on default) and the base directory in which the log files are stored in the target system,
passing the `--base-dir <base directory>` argument (on default, it is `tcc_rt_communication_demo_output` in the home directory).
For the `--input <list of files>` argument we can pass either absolute paths to the log files or relative paths, in the later case, the path is calculated in respect to the base directory.
A passwordless connection from your host system to the target system(s) may come in handy for this mode of operation.
Here is an example command line:
```
python3 tcc_rt_communication_demo_plotter.py --address target-hostname --base-dir tests/tcc_rt_communication_demo_output --user user --input log_e2e.log log_e2e-opt.log
```

## Command-Line Options
```
usage: tcc_rt_communication_demo --profile <profile name> --interface <interface name> [--mode <mode name>] [--no-best-effort] [--config-path <directory with profiles configuration>] [--output-file <output file>] [--exec-path <path to binary executables>] [--target <target>] [--base-time <UTC base time>] [--remote-address <remote-target-address>] [--remote-interface <remote interface name>] [--remote-config-path <remote directory with profiles configuration>] [--remote-exec-path <remote path to binary executables>] [--verbose]

Profile name and interface name (described below) are required.

optional arguments:
  -h, --help            show this help message and exit
  -p PROFILE, --profile PROFILE
                        Define the name of the profile to be executed.
                        The following profiles are supported ('a' for Board A and 'b' for Board B):
                            Basic, with and without optimizations:
                                basic-a-opt
                                basic-a-noopt
                                basic-b-opt
                                basic-b-noopt
                            SISO, with and without optimizations:
                                siso-single-a-opt
                                siso-single-a-noopt
                                siso-single-b-opt
                                siso-single-b-noopt
  -i INTERFACE, --interface INTERFACE
                        Define the name of the network interface to be used for TSN-based data transmission.
  -r REMOTE_INTERFACE, --remote-interface REMOTE_INTERFACE
                        Define the name of the network interface on remote side to be used for TSN-based data transmission. If not specified, the local interface name will be used.
  -m MODE, --mode MODE  Define the operation mode to be used for customization of sample behavior.
                        The following modes are supported:
                            all (default): run all modes/stages: TSN configuration,
                                clock synchronization, and run the sample application
                            setup: configure the network interfaces, (re-)start
                                time syncronization.
                            clock: (Re-)start ptp4l and phc2sys clock synchronization
                            tsn: Configure the network interface and the traffic control for TSN
                            run: Only run the sample application; TSN and clock should already be configured
  -n, --no-best-effort  Do not use Best Effort traffic (iperf3) together with other components of the sample application.
  -c CONFIG_PATH, --config-path CONFIG_PATH
                        Define the directory that contains profiles configuration files.
                        './cfg/' is used by default. The directory should contain 2 subdirectories:
                        'basic/' for basic-<...> profiles configuration files, and
                        'siso-single/' for siso-single-<...> profiles configuration files.
  -g REMOTE_CONFIG_PATH, --remote-config-path REMOTE_CONFIG_PATH
                        Define the directory on remote side that contains profiles configuration files.
                        './cfg/' is used by default. The directory should contain 2 subdirectories:
                        'basic/' for basic-<...> profiles configuration files, and
                        'siso-single/' for siso-single-<...> profiles configuration files.
  -o OUTPUT_FILE, --output-file OUTPUT_FILE
                        Define the file name for storing the output statistics data.
  -e EXEC_PATH, --exec-path EXEC_PATH
                        Define the directory with binary executables such as opcua_server.
                        Sample uses these binary executables from general system directory with binaries as per
                        default and thus they are available by search from PATH environment.
                        Usage of this option prepends provided directory to PATH variable.
  -u REMOTE_EXEC_PATH, --remote-exec-path REMOTE_EXEC_PATH
                        Define the directory on remote side with binary executables such as opcua_server.
                        Sample uses these binary executables from general system directory with binaries as per
                        default and thus they are available by search from PATH environment.
                        Usage of this option prepends provided directory to PATH variable.
  -a REMOTE_ADDRESS, --remote-address REMOTE_ADDRESS
                        Defines the IP address for the 2nd remote target. If provided, the sample is launched
                        first on the remote side, making it possible to launch the whole RT communication Sample from one side.
                        This parameter can only be combined with 'basic-a-opt/noopt' or 'siso-single-a-opt/noopt' profiles.
  -t TARGET, --target TARGET
                        Define type of target board to be used for selection of configuration for execution.
                        Available types are:
                            AUTO (default): autodetect the target
                            EHL: use configuration for Intel Atom® x6000E Series processors
                            TGL-U: use configuration for 11th Gen Intel® Core™ processors
                            TGL-H: use configuration for Intel® Xeon® W-11000E Series processors
  -b BASE_TIME, --base-time BASE_TIME
                        Optional requested base time (UTC) to start communication.
                        If not provided, it is selected automatically
  -v, --verbose         Enable the verbose mode.

Example: tcc_rt_communication_demo --profile basic-a-opt --interface enp1s0 --mode run
```

## Configurations
Consult the README.md file in install/cfg directory for information about currently supported configuration files.

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
