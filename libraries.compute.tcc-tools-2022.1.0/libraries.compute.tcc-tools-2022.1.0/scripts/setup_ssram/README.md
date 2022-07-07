# tcc_setup_ssram

tcc_setup_ssram setup target platform to use Software SRAM technology of Intel(R) hardwawre.

The script does the following:

1. The script enables boot with hypervisor and Real-time Configuration Manager.

2. The script loads the Real-time Configuration Driver

3. The script applies the default platform configuration and does the reboot.


## Usage

tcc_setup_ssram
```
usage: tcc_setup_ssram.sh enable|disable <PROCESSOR_NAME> [-v|--verify] [-h|--help]


Options:
  enable           Enable cache locking capabilities (enable RTCM, driver, create SoftwareSRAM regions) and reboot
  disable          Disable cache locking capabilities (disable RTCM, delete SoftwareSRAM regions) and reboot
  <PROCESSOR_NAME>  Platform to configure. Supported: TGL-U, TGL-H, EHL
Optional arguments:
  -h, --help    Show this help message and exit
  -v|--verify   Do not perform action, but verify that sistem is configured
```
