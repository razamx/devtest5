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

import os
import sys
import math
import shutil
import subprocess
import ctypes
import ctypes.util
import time
import re
from collections import defaultdict, Counter
# hotspot utils
import modules.tcc_measurement_analysis_utils as utils

class tcc_measurement_buffer(ctypes.Structure):
    _fields_ = [('buffer_size', ctypes.c_uint64),
                ('writer_index', ctypes.c_uint64),
                ('reader_index', ctypes.c_uint64)]

class timeval(ctypes.Structure):
    _fields_ = [("tv_sec", ctypes.c_long),
                ("tv_nsec", ctypes.c_long)]

# Lazy initialization for tcc_lib to avoid crash on import if libtcc does not exist
# __tcc_lib_static implements static methods for tcc_lib.
# When we trying to access some field there will be a call to __getattr__ function.
# __getattr__ will initialize tcc_lib and swap tcc_lib content to the lib object.

class __tcc_lib_static(type):

    def __getattr__(cls, key):
        lib=ctypes.CDLL(ctypes.util.find_library("tcc")) #init

        lib.tcc_measurement_buffer_init.argtypes = [ctypes.c_size_t,
                                                        ctypes.c_char_p,
                                                        ctypes.POINTER(ctypes.POINTER(tcc_measurement_buffer))]
        lib.tcc_measurement_buffer_init.restypes = [ctypes.c_int64]

        lib.tcc_measurement_buffer_check.argtypes = [ctypes.c_char_p]
        lib.tcc_measurement_buffer_check.restypes = [ctypes.c_int]
        lib.tcc_measurement_buffer_get.argtypes = [ctypes.POINTER(tcc_measurement_buffer)]
        lib.tcc_measurement_buffer_get.restypes = [ctypes.c_uint64]

        lib.tcc_measurement_get_buffer_size_from_env.argtypes = [ctypes.c_char_p]
        lib.tcc_measurement_get_buffer_size_from_env.restypes = [ctypes.c_size_t]

        lib.tcc_measurement_convert_clock_to_timespec.argtypes = [ctypes.c_uint64]
        lib.tcc_measurement_convert_clock_to_timespec.restype = timeval

        lib.tcc_measurement_convert_timespec_to_clock.argtypes = [timeval]
        lib.tcc_measurement_convert_timespec_to_clock.restype = ctypes.c_uint64
        # move lib content to tcc_lib
        for lib_method_name, lib_method in lib.__dict__.items():
            setattr(cls, lib_method_name, lib_method)
        # return method that was called
        return getattr(cls, key)

class tcc_lib(object, metaclass=__tcc_lib_static):
    None

def clocks_to_time_units(clocks, time_units):
    time_units.lower()
    if time_units == 'ns':
        time_spec = tcc_lib.tcc_measurement_convert_clock_to_timespec(clocks)
        return int(time_spec.tv_sec * 1000000000 + time_spec.tv_nsec)
    elif time_units == 'us':
        time_spec = tcc_lib.tcc_measurement_convert_clock_to_timespec(clocks)
        return int(time_spec.tv_sec * 1000000 + time_spec.tv_nsec / 1000)
    else:
        return int(clocks)

def time_units_to_clocks(time_interval, time_units):
    time_units.lower()
    if time_units == 'ns':
        time_spec = timeval()
        time_spec.tv_nsec = time_interval % 1000000000
        time_spec.tv_sec = int(time_interval / 1000000000)
        return tcc_lib.tcc_measurement_convert_timespec_to_clock(time_spec)
    elif time_units == 'us':
        time_spec = timeval()
        time_spec.tv_nsec = (time_interval * 1000) % 1000000000
        time_spec.tv_sec = int(time_interval / 1000000)
        return tcc_lib.tcc_measurement_convert_timespec_to_clock(time_spec)
    else:
        return int(time_interval)

class Logger:
    def __init__(self, filename):
        self._term = sys.stdout
        try:
            self._file = open(filename, 'w')
        except IOError:
            print("Could not read file: " + filename)

    def write(self, message):
        self._term.write(message)
        self._file.write((re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')).sub('', message))

    def flush(self):
        pass
        #for compatible

class Measurer:
    def __init__(self, name, buffer_size, deadline):
        self._name = name
        self._buffer_size = buffer_size
        self._deadline = deadline
        self._buffer = ctypes.pointer(tcc_measurement_buffer(0, 0, 0))
        self._count_of_read_data = 0
        self._deadlines_count = 0

        # Problem: potentially shared memory leak
        # When invalid name is passed, will create shared memory which won't
        # be destroyed by app
        # tcc_lib.tcc_measurement_buffer_init(ctypes.c_size_t(self._buffer_size),
        #                                    ctypes.c_char_p(self._name.encode()),
        #                                    ctypes.pointer(self._buffer))

    total_deadlines_count = 0

    @property
    def name(self):
        return self._name

    @property
    def buffer_size(self):
        return self._buffer_size

    @property
    def read_data_count(self):
        return self._count_of_read_data

    @property
    def deadline(self):
        return self._deadline

    @property
    def deadlines_count(self):
        return self._deadlines_count

    def check_buffer(self):
        return tcc_lib.tcc_measurement_buffer_check(self._name.encode())

    def create_buffer(self):
        err = tcc_lib.tcc_measurement_buffer_init(ctypes.c_size_t(self._buffer_size),
                                                ctypes.c_char_p(self._name.encode()),
                                                ctypes.pointer(self._buffer))
        if err < 0:
            utils.colored_print( '{}: fail to create buffer, error: {}'.format(self.name, err),
                                      style=utils.AnsiStyles.FAIL)
            return False
        return True

    def has_data_to_read(self):
        # Check if writer's index differs from reader's index
        return self._buffer.contents.writer_index != self._buffer.contents.reader_index

    def get_latency(self):
        return tcc_lib.tcc_measurement_buffer_get(self._buffer)

    def count_latencies(self, latency):
        if self.is_exceed_deadline(latency):
            Measurer.total_deadlines_count += 1
            self._deadlines_count += 1
        if latency != 0:
            self._count_of_read_data += 1

    def is_exceed_deadline(self, latency):
        return latency > self._deadline and self._deadline != 0

    def print_latency(self, latency, table_width, time_units='clk', deadline_flag=False):
        if latency <= 0:
            return
        out_str = "{0}{1}{2}".format(self._name,
                                     ' ' * (table_width - len(self._name) - 8),
                                     clocks_to_time_units(latency, time_units))
        if self.is_exceed_deadline(latency):
            print('\033[31m' + out_str + " Latency exceeding deadline\033[0m")
        elif not deadline_flag:
            print(out_str)

def measurement_instance_from_string(string, time_units):
    """
    Parses measurement string with format
    <name>:<buffer_size>[:deadline]
    and creates Measurer
    """
    values = string.split(':')
    name = values[0].strip()
    buffer_size = int(values[1].strip())
    deadline = 0
    if len(values) >= 3:
        deadline = int(values[2].strip())
    return Measurer(name, buffer_size, time_units_to_clocks(deadline, time_units))


def print_table_header(measurement_instances, width_of_table, time_units):
    for measurement_instance in measurement_instances:
        if measurement_instance.deadline != 0:
            print('Monitoring "{0}" with deadline {1} {2}.'.format(measurement_instance.name,
                                                                    clocks_to_time_units(measurement_instance.deadline, time_units),
                                                                    time_units))
    print('\n{0}\nName{1}Latency({2})\n{0}'.format('-' * (width_of_table + 5), ' ' * (width_of_table - 12), time_units))


def init_env(time_units, measurement_instances_string):
    setenv_res = utils._setenv_time_units(time_units)
    if setenv_res != 0:
        return setenv_res
    setenv_res = utils._setenv_buffer(measurement_instances_string, 'monitor')
    if setenv_res != 0:
        return setenv_res
    setenv_res = utils._setenv_shared_memory()
    if setenv_res != 0:
        return setenv_res
    return 0

def monitor(measurement_instances_string, dump_file, time_units, deadline_flag, app, app_args):
    """
    Main function for monitoring data:
	Set environment
        Run application
        Get and print data from buffer of measurement results
	Catch deadline situation

    Arguments:
    measurement_instances_string -- measurement instances' names for which data is to be collected
    dump_file -- history data dump file for collected measurement data
    deadline_flag -- flag indicating whether to print only deadline violations
    app -- application path
    app_args -- application command-line arguments as list
    """
    setenv_res = init_env(time_units, measurement_instances_string)
    if setenv_res != 0:
        return setenv_res

    if shutil.which(app) is None:
        utils.colored_print(
            'Application [{0}] does not exist'.format(os.path.basename(app)),
            style=utils.AnsiStyles.FAIL
        )
        return 1

    measurement_instances_list = [measurement_instance_from_string(value, time_units) for value in measurement_instances_string.split(',')]

    # Create shared memory buffers
    for measurement_instance in measurement_instances_list:
        if not measurement_instance.create_buffer():
            return 1

    command = [app] + app_args
    completed_proc = subprocess.Popen(command,
                                      stderr=subprocess.PIPE,
                                      stdout=subprocess.PIPE)

    print('\nMONITORING OUTPUT:\n')
    width_of_table = max((len(measurement_instance.name) for measurement_instance in measurement_instances_list)) + 15
    print_table_header(measurement_instances_list, width_of_table, time_units)
    if dump_file != '':
        sys.stdout = Logger(dump_file)

    # Performs monitoring
    reader_flag = True #need to read
    while reader_flag or completed_proc.poll() is None:
        reader_flag = False
        for measurement_instance in measurement_instances_list:
            latency = measurement_instance.get_latency()
            measurement_instance.count_latencies(latency)
            measurement_instance.print_latency(latency, width_of_table, time_units, deadline_flag)
            reader_flag = reader_flag or latency != 0 or measurement_instance.has_data_to_read()

    if Measurer.total_deadlines_count == 0 and deadline_flag:
        print("NO DEADLINE VIOLATIONS HAPPENED")
    print()
    for measurement_instance in measurement_instances_list:
        count_str = f'[{measurement_instance.name}] Count of read data: {measurement_instance.read_data_count}'
        deadline_str = \
            f' (deadline violations: {measurement_instance.deadlines_count})' \
            if measurement_instance.deadline != 0 \
            else ''

        print(f'{count_str}{deadline_str}')

    print('\n'+'-'*30)
    out, err = completed_proc.communicate()
    print('\nAPPLICATION OUTPUT:\n', flush=True)
    print(utils.to_ascii(out))
    return 0
