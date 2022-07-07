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

class str_eq_re(str):
    def __eq__(self, other):
        result = False
        try:
            result = str(self) == str(other) or bool(re.fullmatch(str(other), str(self)))
        except:
            print("DEBUG: self='%s', other='%s'" % (str(self), str(other)))
        return result
    def __hash__(self):
        return hash(str(self))
