# TCC installation configuration file format specification

This file describes the format for the TCC installation configuration files.
It is intended to be used by maintainers of this 'intel-tsn-distro' package.
The TCC installation configuration files are used by 'tcc_setup.py' script
in order to customize the installation process for a particular HW platform.
The scripts can be found in 'configs' sub-folder in the installation folder
with 'tcc_setup.py' script.

## List of elements supported in the TCC installation configuration files
The TCC installation configuration file constitutes a list of configuration
lines. Each line can be a comment or a rule or statement, as follows:
1. Comments
Any line starting with '#' is a comment and will be ignored by the script;
Note: only whole lines should be used for comments, adding a comment after
other configuration elements is not supported.
2. Passlisting installing elements
Any configuration line starting with 'PASSLIST:' defines a passlisting
rule for the installing components. The 'PASSLIST:' prefix is followed by
wildcard-based rule for passlisting elements. The TCC installation configuration
file can have any reasonable number of 'PASSLIST:' configuration lines.
Any installing element that match any passlisting rule will be installed.
Any element that does not match any passlisting rule will not be installed.
An installing element may represent a file or symbolic link. If a file falls
under any passlisting rule, the folder(s) in which it is located are also
(possibly - implicitly) passlisted.
3. Custom permissions
If a line starts with 'PERMIT:' prefix, it defines a rule for updating
installing elements permissions after installation. The 'PERMIT:' refix
is followed by 2 more fields: the requested mode for the elements in octal
numeric format, similar to what we pass in 'chmod' command, and the
wildcard-based rule for matching installing element(s) for which the updated
permissions will be applied. The 2 extra fields are separated with ':' character.
The custom mode will be selected in the first from the top 'PERMIT:' configuration
line for which the element will match. So, more specific (detailed) rules should
go first, and more generic rule should go last.
## Example
Here is a example configuration file demonstrating all configuration features:
```
# Begining of the configuration file, this is just a comment
# The following line passlisting all the elements in 'usr/' folder and its sub-folders:
PASSLIST:usr/*
# The following line defines '700' permission for 'usr/bin/restricted-application' executable
PERMIT:700:usr/bin/restricted-application
# The following line defines '777' permission for all other elements in 'usr/bin', excluding
# the directory 'usr/bin' itself
PERMIT:777:usr/bin/*
# The following line defines '766' permission for element 'usr/bin', all other elements
# already covered by the previous rule. This rule will also be applied to other elements,
# like: 'usr/bin-new' and all its sub-components (folders/files/links)
PERMIT:766:usr/bin*
# End of configuration file
```
