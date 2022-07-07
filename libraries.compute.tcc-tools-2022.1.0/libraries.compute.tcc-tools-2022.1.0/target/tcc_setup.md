# Install/Uninstall Script

This file describes the Intel® Time Coordinated Computing Tools (Intel® TCC Tools)
install/uninstall script, `tcc_setup.py`.
Python3 is needed to execute the script.

The script can run in 2 modes:
* Install: `-i <archive-filename>` option
* Uninstall: `-u` option

## Install Intel® TCC Tools

The script can install Intel® TCC Tools software on the target system.

To install, pass the `-i <archive-filename>` or `--install <archive-filename>` option to the script.

Example:
* To install:

```
./tcc_setup.py -i tcc_tools_target_<version>.tar.gz
```

You can also specify a particular hardware platform by using the following option:
* `--platform <platform>` or `-p <platform>`

Supported platforms:
* `ehl`: The script will install only features supported by Intel Atom® x6000E Series processors
* `tgl-u`: The script will install only features supported by 11th Gen Intel® Core™ processors
* `default`: The script will install all files (default option)

Example:
* To specify a platform:

```
./tcc_setup.py -i tcc_tools_target_<version>.tar.gz -p <platform>
```

Additionally, you can customize the installation directories by using the following optional arguments:
* `--prefix <install-path>` defines the default installation prefix. If it is not provided, `/usr` will be used.
* `--prefix-bin <bin-install-path>` defines the installation prefix for the `bin` folder (with executables). If it is not provided,
           the following installation prefix will be used for bin content: `<install-path>/bin`.
* `--prefix-lib <lib-install-path>` defines the installation prefix for the `lib` folder (with shared libraries). Tf it is not provided,
           the following installation prefix will be used for lib content: `<install-path>/lib64`.
* `--prefix-share <share-install-path>` defines the installation prefix for the `share` folder (additional shared content). If it is not
           provided, the following installation prefix will be used for shared content: `<install-path>/share`.
* `--prefix-include <include-install-path>` defines the installation prefix for the `include` folder (header files). If it is not provided,
           the following installation prefix will be used for include content: `<install-path>/include`.

Before beginning, the installation script verifies whether Intel® TCC Tools software is already installed.
If the script finds an existing version, the script will ask you whether to proceed with removing the old version and installing the new one.

If you agree, the script will attempt to remove the old version. If it fails to remove the old version, the script will print a command for manual removal. You should remove the old version manually using the provided command and rerun the script in the install mode again.

Also, the script checks whether the target system contains conflicting elements in the file system,
that would be overridden by the new installation. If such elements exist, installation stops. The script lists the conflicting
elements. You can either remove the conflicting elements, or choose to install components to a different location.

## Uninstall Intel® TCC Tools

The script can uninstall all data that was installed by the script.

To uninstall, pass the `-u` or `--uninstall` option to the script.

Additionally, you can pass the `-f` or `--force` option to uninstall all folders created in the installation mode, even
if they are not empty. All user content in the folders will be deleted.
All folders that existed before installation will not be deleted, even with the `-f`/`--force` option.

Example:
* To uninstall:

```
./tcc_setup.py -u
```

* To uninstall and force removal of all previously created folders:

```
./tcc_setup.py -u -f
```

If the script cannot remove some elements (for example, the current user does not have permissions to
delete components), it fails with an error, asking the user to run the command with superuser privileges.

## Internals Overview

The installation script uses platform-specific configuration files located in the ./configs directory.

The installation script stores the installation state in the installation configuration file in:
* ${HOME}/.tcc_tools/install-info.cfg

Each run of the install/uninstall script creates a new log file in the following directory:
* ${HOME}/.tcc_tools/

The log filename usually appears in console output. The log filename does not appear in console output when users attempt to run the script without required arguments or use unsupported arguments.

The log can be used to analyze progress and failures.

The installation script stores a copy of tcc_setup.py in:
* ${HOME}/.tcc_tools/

You can use this tcc_setup.py to uninstall the product in the future.

# Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
