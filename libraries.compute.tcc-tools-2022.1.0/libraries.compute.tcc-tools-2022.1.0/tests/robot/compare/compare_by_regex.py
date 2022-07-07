# /*******************************************************************************
# INTEL CONFIDENTIAL
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

import re

def is_file_compatible_with_pattern (pattern_file, file_to_check):
    with open(pattern_file, 'r') as (f_pattern), open(file_to_check, 'r') as (f_to_check):
        pattern_string = f_pattern.read()
        string_to_check = f_to_check.read()
    f_pattern.close()
    f_to_check.close()
    if re.search(pattern_string, string_to_check):
        return True
    else:
        return False
