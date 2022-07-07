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
from enum import IntEnum
import os
import time
import sys
import traceback

from typing import NoReturn
from decorators import singleton_class


class log_level(IntEnum):
    PANIC = 0
    ERROR = 1
    WARNING = 2
    NOTICE = 3
    DEBUG = 4
    DEBUG2 = 5


# Singleton class for tracing facility
@singleton_class
class traces():
    def __init__(self, level=log_level.NOTICE):
        self.__idx_2_name_map = {
            log_level.PANIC: 'FATAL ERROR:',
            log_level.ERROR: 'ERROR:',
            log_level.WARNING: 'WARNING:',
            log_level.NOTICE: 'NOTICE:',
            log_level.DEBUG: 'DEBUG:',
            log_level.DEBUG2: 'DEBUG2'
        }
        if level not in self.__idx_2_name_map:
            self.__log_level = log_level.NOTICE
        else:
            self.__log_level = level

    def log(self, idx, *args):
        if idx > self.__log_level:
            return
        data = ''
        if self.__log_level == log_level.DEBUG2:
            data += f'{time.ctime()} '
        prefix = self.__get_log_prefix(idx)
        data += f'{prefix}'
        for arg in args:
            data += f' {arg}'
        print(data)

    def debug2(self, *args):
        self.log(log_level.DEBUG2, *args)

    def debug(self, *args):
        self.log(log_level.DEBUG, *args)

    def notice(self, *args):
        self.log(log_level.NOTICE, *args)

    def warning(self, *args):
        self.log(log_level.WARNING, *args)

    def error(self, *args):
        self.log(log_level.ERROR, *args)

    def panic(self, *args) -> NoReturn:
        self.log(log_level.PANIC, *args)
        if self.__log_level == log_level.DEBUG2:
            # Avoid to capture extract_stack() itself
            stack = traceback.extract_stack()[:-1]
            self.log(log_level.DEBUG2, "Traceback:\n", ''.join(traceback.format_list(stack)))
        sys.exit(os.EX_SOFTWARE)

    def set_log_level(self, idx):
        if idx in self.__idx_2_name_map:
            self.__log_level = idx

    def __get_log_prefix(self, idx):
        if idx in self.__idx_2_name_map:
            return self.__idx_2_name_map[idx]
        else:
            message = f'Bad log level {idx} was provided'
            self.log(log_level.ERROR, message)
            return 'UNKNOWN LOG_LEVEL'


if __name__ == '__main__':
    traces().panic('this file is component of RTC sample and is not intended to be executed directly')
