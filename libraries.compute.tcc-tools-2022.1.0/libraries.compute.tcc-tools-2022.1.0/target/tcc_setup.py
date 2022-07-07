#!/usr/bin/env python3
# /*******************************************************************************
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/
# flake8: noqa CCR001
import os
import shutil
import fnmatch
import tarfile
import argparse
from datetime import datetime
from typing import List

log_file = None
error_reported = False

config_lines: List[str] = []
install_cfg_file = None
install_cfg_filename = ""

# Should be replaced by list if name was updated
setup_script_name = "tcc_setup.py"

home_dir = os.getenv("HOME")
if home_dir is None:
    print("Error: cannot get HOME path from environment")
    exit(1)

config_dir_base = ".tcc_tools"
config_dir = os.path.join(home_dir, config_dir_base)

install_prefix = ""
install_prefix_bin = ""
install_prefix_lib = ""
install_prefix_share = ""
install_prefix_include = ""


def log_line(line):
    global log_file

    if log_file is not None:
        log_file.write("{}\r\n".format(line))
        log_file.flush()


def add_install_item(item, item_type):
    global install_cfg_file
    if install_cfg_file is not None:
        install_cfg_file.write("{}:{}\r\n".format(item_type, item))


def report_preinstallation_error(body):
    global error_reported

    if (not error_reported):
        error_reported = True
        print("Error: pre-installation check failed, the following issue(s) detected:")
    print("* {}".format(body))


def create_passlist(archive_items):
    global config_lines

    passlist_substr = "PASSLIST:"
    passlist = []
    config_passlist = []
    # Build the base passlist first
    for line in config_lines:
        if (line.startswith(passlist_substr)):
            pitem = line[len(passlist_substr):].rstrip()
            log_line("Config passlist item: \"{}\"".format(pitem))
            config_passlist.append(pitem)
    # Now, iterate through the 'archive_items', check whether the items are in the config passlist
    for item in archive_items:
        passlisted = False
        for pitem in config_passlist:
            if (fnmatch.fnmatch(item.name, pitem)):
                passlisted = True
                break
        if (passlisted):
            log_line("Added item to passlist: \"{}\"".format(item.name))
            passlist.append(item.name)

    # Finally, search for missing items that are required for passlist items
    for item in archive_items:
        if (item.name not in passlist and item.isdir()):
            for pitem in passlist:
                if (pitem.startswith(item.name)):
                    passlist.append(item.name)
                    log_line("Added parent directory to passlist: \"{}\"".format(item.name))
                    break

    return passlist


def apply_permissions(archive_items):
    # Apply custom permissions for the items in the archive first
    for item in archive_items:
        mode_request = custom_mode_request(item.name)
        if (mode_request is None):
            continue

        item_name = tcc_install_update_prefix(item.name)
        log_line("Updating permissions for item [{}] to mode: [{}]".format(item_name, oct(mode_request)[2:]))
        os.chmod(item_name, mode_request)


def tcc_install_update_prefix(itemname):
    global install_prefix
    global install_prefix_bin
    global install_prefix_lib
    global install_prefix_share
    global install_prefix_include

    if (itemname.startswith('usr/bin')):
        return install_prefix_bin + itemname[7:]
    elif (itemname.startswith('usr/lib64')):
        return install_prefix_lib + itemname[9:]
    elif (itemname.startswith('usr/share')):
        return install_prefix_share + itemname[9:]
    elif (itemname.startswith('usr/include')):
        return install_prefix_include + itemname[11:]
    elif (itemname.startswith('usr')):
        return install_prefix + itemname[3:]
    else:
        log_line("Error: unexpected path in archive: \"{}\", ignoring".format(itemname))
        exit(1)  # noqa: R503


def tcc_install_open_config_file(platform):
    global config_lines

    script_path = os.path.abspath(os.path.split(__file__)[0])
    config_filename = os.path.join(script_path, 'configs', "install-{}.cfg".format(platform))
    log_line("Reading configuration in \"{}\"".format(config_filename))
    try:
        with open(config_filename, 'r') as config_file:
            config_lines = config_file.readlines()
    except IOError:
        log_line("Error: failed to read config file \"{}\"".format(config_filename))
        exit(1)


def tcc_install_create_install_configuration_file():
    global install_cfg_filename
    global install_cfg_file

    log_line("Opening install configuration file: \"{}\"".format(install_cfg_filename))
    try:
        install_cfg_file = open(install_cfg_filename, "x")
    except OSError:
        log_line("Error: failed to create installation config file: \"{}\"".format(install_cfg_filename))
        print("Error: failed to create installation config file, already installed?")
        exit(1)


def custom_mode_request(item_name):
    for line in config_lines:
        line = line.rstrip()
        if (not line.startswith('PERMIT:')):
            continue
        mode, pattern = line.split(':')[1:]
        if (not fnmatch.fnmatch(item_name, pattern)):
            continue
        # Found a config line with setting for the item, return it
        return int(mode, 8)
    # No custom mode found, return None
    return None


def tcc_install_directory(ifile, dir_item):
    name = dir_item.name
    # Check whether the parent directories exist, create, if needed
    base_dir, _ = os.path.split(name)
    # 'base_dir' should exist, if it does not, create it and add to the installation list
    missing_folders = []
    while (not os.path.isdir(base_dir)):
        missing_folders.append(base_dir)
        base_dir, _ = os.path.split(base_dir)
    for folder in missing_folders[::-1]:
        os.mkdir(folder)
        add_install_item(folder, "D")
        log_line("Created missing directory: [{}]".format(folder))

    # Now, extract the current directory item
    if os.path.isdir(dir_item.name):
        path_type = "E"
        log_line("Existing directory: [{}]".format(dir_item.name))
    else:
        path_type = "D"
        log_line("Directory: [{}]".format(dir_item.name))
        ifile.extract(dir_item)

    add_install_item(dir_item.name, path_type)


def tcc_check_install(items, passlist):
    good = True
    for item in items:
        item_name = item.name
        # Check the item against the passlist
        if (item_name not in passlist):
            # Skip an item that is not in the passlist
            continue
        # Update the item prefix
        updated_item_name = tcc_install_update_prefix(item_name)

        if (item.isdir()):
            # Check the directory
            if (os.path.exists(updated_item_name) and not os.access(updated_item_name, os.W_OK)):
                # We have a directory that does not have Write permission, cannot continue
                good = False
                log_line("Error: directory [{}] is not writable".format(updated_item_name))
                report_preinstallation_error(
                    "Directory [{}] already exists, but it is not writable".format(updated_item_name))
        elif (item.isfile()):
            # Check the file
            if (os.path.exists(updated_item_name)):
                good = False
                log_line("Error: file: [{}] already exists, installation failed".format(updated_item_name))
                report_preinstallation_error(
                    "File [{}] already exists and would be overwritten by installation".format(
                        updated_item_name))
        elif (item.issym()):
            if (os.path.exists(updated_item_name)):
                good = False
                log_line("Error: symbolic link: [{}] already exists, installation failed".format(
                    updated_item_name))
                report_preinstallation_error(
                    "Symbolic link [{}] already exists and would be overwritten by installation".format(
                        updated_item_name))
    return good


def symlink_correction(extracted_symlink, original_symlink):
    source = os.path.normpath(os.path.join(os.path.dirname(original_symlink), os.readlink(extracted_symlink)))
    source = tcc_install_update_prefix(source)
    os.remove(extracted_symlink)
    os.symlink(source, extracted_symlink)


def open_tarfile(filename):
    try:
        ifile = tarfile.open(filename)
        return ifile
    except OSError:
        log_line("Error: failed to open the archive file: \"{}\"".format(filename))
        print("Error: failed to open the archive file:\"{}\"".format(filename))
        exit(1)


def ask_yes_no_question(question, default=None):
    if (default is not None) and (default is not True) and (default is not False):
        raise TypeError("default should be True, False or None")

    valid = {"yes": True, "y": True, "no": False, "n": False, "": default}
    prompt = question

    if default is None:
        prompt += " [y/n]"
    elif default is True:
        prompt += " [Y/n]"
    elif default is False:
        prompt += " [y/N]"

    prompt += ": "

    answer = str(input(prompt)).lower()
    res = valid.get(answer, None)

    while res is None:
        print("\'{}\' is not a valid value. Please enter \'yes\' or \'no\'".format(answer))

        answer = str(input(prompt)).lower()
        res = valid.get(answer, None)

    return res


def tcc_install(filename):
    global config_dir
    global install_cfg_file
    global install_cfg_filename

    # Check whether the installation file already exists
    if os.path.exists(install_cfg_filename):
        log_line("Info: the installation configuration file already exists")

        question = ("Intel(R) TCC Tools are already installed."
                    " Cannot install a new version until the old version is removed. "
                    " Do you want to remove the old version and install new?")

        res = ask_yes_no_question(question, default=True)
        if res is False:
            log_line("Info: declined removing the old version and installing new")
            print("Declined removing the old version and installing new")
            print("Exiting ...")
            exit(0)
        else:
            log_line("Info: accepted removing the old version and installing new")
            tcc_uninstall(force=False)

    # Check whether the archive file exists
    if not os.path.isfile(filename):
        log_line("Error: cannot find the installation archive file")
        print("Error: cannot find the installation archive file")
        exit(1)

    # Open the archive file
    ifile = open_tarfile(filename)
    items = ifile.getmembers()

    # Create the installation configuration file
    tcc_install_create_install_configuration_file()

    passlist = create_passlist(items)
    if not tcc_check_install(items, passlist):
        print("Error: installation check failed, check the log file for details")
        log_line("Warning: failed installation check, rolling back installation")
        try:
            os.remove(install_cfg_filename)
        except OSError:
            log_line(
                "Error: failed to remove the installation configuration file after failed installation check")
            pass
        exit(1)

    for item in items:
        # Check the item against the passlist
        if (item.name not in passlist):
            log_line("Item is not in the passlist. Skipped: \"{}\"".format(item.name))
            continue
        # Update the item prefix
        original_name = item.name
        item.name = tcc_install_update_prefix(item.name)

        if (item.isdir()):
            tcc_install_directory(ifile, item)
        elif (item.isfile()):
            log_line("File: [{}]".format(item.name))
            ifile.extract(item)
            add_install_item(item.name, "F")
        elif (item.issym()):
            log_line("Symlink: [{}]".format(item.name))
            ifile.extract(item)
            symlink_correction(item.name, original_name)
            add_install_item(item.name, "S")
        else:
            log_line("Other: [{}]".format(item.name))
            ifile.extract(item)
            add_install_item(item.name, "O")
        # Restore the item name for further handling
        item.name = original_name

    # Apply custom permissions
    apply_permissions(items)

    # Copy this script to the config folder
    shutil.copy(__file__, config_dir)


def tcc_uninstall(force):

    if not os.path.exists(install_cfg_filename):
        print("Error: cannot find installed components: "
              "config doesn't exist by expected path \"{}\"".format(install_cfg_filename))
        log_line("Error: config file \"{}\" doesn't exist".format(install_cfg_filename))
        exit(1)
    else:
        old_setup_script = os.path.abspath(os.path.join(config_dir, setup_script_name))
        current_setup_script = os.path.abspath(__file__)
        log_line("Info: old setup script path \"{}\"".format(old_setup_script))
        log_line("Info: current setup script path \"{}\"".format(current_setup_script))
        if os.path.exists(old_setup_script):
            log_line("Info: old setup script exists")
            if old_setup_script != current_setup_script:
                log_line("Info: called setup script and old setup script are not same")
                import subprocess
                command = [old_setup_script, "-u"]
                if force:
                    command.append("-f")
                log_line("Info: called subprocess with command: \"{}\"".format(command))
                completed_proc = subprocess.run(command, capture_output=True)
                if completed_proc.returncode != 0:
                    log_line("Error: subprocess filed with code \"{}\"".format(completed_proc.returncode))
                    print("Error: automatic removing is failed. "
                          "Try to call \"{}\" manually".format(" ".join(command)))
                    exit(1)
                else:
                    log_line("Info: removed with old setup script")
                    return
            else:
                log_line("Info: called setup script and old setup script are same")
        else:
            log_line("Warning: old setup script doesn't exist")
        log_line("Info: follow default uninstall flow")

    lines = []
    if (force):
        log_line("Uninstall request started in forced mode")
    else:
        log_line("Uninstall request started in normal mode")
    log_line("Opening installation configuration file: \"{}\"".format(install_cfg_filename))
    try:
        with open(install_cfg_filename, 'r') as install_cfg_file:
            lines = install_cfg_file.readlines()[::-1]
    except IOError:
        print("Error: cannot find installed components")
        log_line("Error: failed to open installation config file \"{}\"".format(install_cfg_filename))
        exit(1)

    have_remnants = False
    for line in lines:
        line = line.rstrip()
        itemname = line[2:]
        try:
            if (line.startswith('D:')):
                log_line("Delete directory: [{}]".format(itemname))
                try:
                    os.rmdir(itemname)
                except OSError:
                    if (force):
                        log_line("Directory is not empty, remove the whole tree: [{}]".format(itemname))
                        shutil.rmtree(itemname, ignore_errors=True)
                    pass
            elif (line.startswith('F:')):
                # Handling a file
                log_line("Delete file: [{}]".format(itemname))
                os.remove(itemname)
            elif (line.startswith('S:')):
                # Handling a SymLink
                log_line("Delete link: [{}]".format(itemname))
                os.remove(itemname)
        except FileNotFoundError:
            log_line("Info: item \"{}\" already deleted".format(line))
            pass
        except OSError as error:
            have_remnants = True
            log_line("Warning: failed to remove element \"{}\", error: \"{}\"".format(line, str(error)))
            pass

    # Error case, some files/links could not be deleted, report the error
    if have_remnants:
        print("Error: some items cannot be deleted, check the log file for details, "
              "try to re-run the uninstall command with superuser permissions")
        exit(1)

    # All the previously installed files are gone, remove the installation configuration file
    try:
        os.remove(install_cfg_filename)
    except OSError as error:
        print("Error: failed to remove the installation config file. "
              "Try to remove it manually: \"rm {}\"".format(install_cfg_filename))
        log_line("Error: failed to remove the installation config file: \"{}\", error: \"{}\"".format(
            install_cfg_filename, str(error)))
        exit(1)


def parse_cli():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description="The script is used to install/uninstall "
                    "Intel(R) TCC Tools software on the target system. ")
    install_group = parser.add_mutually_exclusive_group(required=True)
    install_group.add_argument('-i', '--install', dest='file', type=str, help="Install Intel(R) TCC Tools")
    install_group.add_argument('-u', '--uninstall', help="Uninstall Intel(R) TCC Tools", action='store_true')
    parser.add_argument('-r', '--prefix', dest='prefix', type=str, default='/usr', help="Installation prefix")
    parser.add_argument(
        '-b', '--prefix-bin', dest='prefix_bin', type=str,
        required=False, help="Installation prefix for 'bin' directory")
    parser.add_argument(
        '-a', '--prefix-include', dest='prefix_include', type=str,
        required=False, help="Installation prefix for 'include' directory")
    parser.add_argument(
        '-s', '--prefix-share', dest='prefix_share', type=str,
        required=False, help="Installation prefix for 'share' directory")
    parser.add_argument(
        '-l', '--prefix-lib', dest='prefix_lib', type=str,
        required=False, help="Installation prefix for 'lib' directory")
    parser.add_argument('-f', '--force', help="Force actions", action='store_true')
    parser.add_argument(
        '-p', '--platform', dest='platform', type=str, default='default',
        help="Name of platform on which package will be installed", choices=['tgl-u', 'tgl-h', 'ehl', 'default'])
    res = parser.parse_args()
    return res


def parse_prefix(install_prefix, special_prefix, default):
    output = ""
    if special_prefix is not None:
        output = os.path.abspath(special_prefix)
        log_line("Apply user installation prefix for '{}' directory: \"{}\"".format(default, output))
    elif install_prefix is not None:
        output = os.path.join(install_prefix, default)
        log_line("Apply default installation prefix for '{}' directory: \"{}\"".format(default, output))
    else:
        log_line("Internal error: default and special install prefixes are None "
                 "for '{}' directory provided".format(default))
        exit(1)
    return output


def parse_prefixes(config):
    global install_prefix
    global install_prefix_bin
    global install_prefix_include
    global install_prefix_lib
    global install_prefix_share

    # Check the global installation prefix
    install_prefix = ""
    if config.prefix is not None:
        install_prefix = os.path.abspath(config.prefix)
        log_line("Installation prefix: \"{}\"".format(install_prefix))

    install_prefix_bin = parse_prefix(install_prefix, config.prefix_bin, "bin")
    install_prefix_include = parse_prefix(install_prefix, config.prefix_include, "include")
    install_prefix_share = parse_prefix(install_prefix, config.prefix_share, "share")
    install_prefix_lib = parse_prefix(install_prefix, config.prefix_lib, "lib64")
    log_line("Install prefix: {}".format(install_prefix))
    log_line("Binary install prefix: {}".format(install_prefix_bin))
    log_line("Library install prefix: {}".format(install_prefix_lib))
    log_line("Share install prefix: {}".format(install_prefix_share))
    log_line("Include install prefix: {}".format(install_prefix_include))


def main():
    global log_file
    global config_dir
    global install_cfg_file
    global install_cfg_filename

    # Parse CLI first
    cli_config = parse_cli()

    # Determine the configuration directory, create it if needed
    if (not os.path.isdir(config_dir)):
        os.mkdir(config_dir)

    # Create the log file
    dt = datetime.now()
    dt_string = dt.strftime("%Y-%m-%d_%H-%M-%S")
    log_filename = os.path.join(config_dir, "{}_info.log".format(dt_string))
    print("Log file: [{}]".format(log_filename))
    log_file = open(log_filename, "a")
    log_line("Created log file: \"{}\"".format(log_filename))
    log_line("CLI arguments: \"{}\"".format(str(cli_config)))
    install_cfg_filename = os.path.join(config_dir, "install-info.cfg")

    # Handle the user request
    if (not cli_config.uninstall):
        # ############################
        # Handling 'install' request
        # ############################
        platform = cli_config.platform
        tcc_install_open_config_file(platform)
        parse_prefixes(cli_config)
        tcc_install(cli_config.file)
    else:
        # ############################
        # Handling 'uninstall' request
        # ############################
        tcc_uninstall(cli_config.force)

    # Close log descriptors
    if log_file is not None:
        log_file.close()
    if install_cfg_file is not None:
        install_cfg_file.close()


if __name__ == '__main__':
    main()
