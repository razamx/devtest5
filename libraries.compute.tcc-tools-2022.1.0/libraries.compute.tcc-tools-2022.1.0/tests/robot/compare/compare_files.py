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

from str_eq_re import str_eq_re
import os

NON_BREAKING_SPACES="&nbsp;"
GT="&gt;"
LT="&lt;"
COLOR_EQ = '#A8FFA8'
COLOR_NEQ = '#FFA8A8'
COLOR_MISSED = '#FFA8BE'
COLOR_NOT_COVERED = '#FFFFA8'
TABLE = '''
<!DOCTYPE html>
<html>
    <head> Compare between %s and %s </head>
    <body>
        <table border=2>
            <tr>
                <th>#</th>
                <th>Current output</th>
                <th>Expected output</th>
            </tr>
%s
        </table>
%s
%s
    </body>
</html>
'''

ROW = ' ' * 16 + '<tr bgcolor="%s"><td>%d</td><td>%s</td><td>%s</td></tr>\n'

FILES_LEN = '''
        <table border=2>
            <tr>
                <th>Current lenght</th>
                <th>Expected lenght</th>
            </tr>
                <tr bgcolor="%s"><td>%d</td><td>%d</td></tr>
        </table>
'''

LEGENDS = '''
        <table>
            <tr>
                <th>Legends</th>
            </tr>
            <tr>
                <th>Colors</th>
            </tr>
                <tr bgcolor="%s"><td>%s</td></tr>
                <tr bgcolor="%s"><td>%s</td></tr>
                <tr bgcolor="%s"><td>%s</td></tr>
                <tr bgcolor="%s"><td>%s</td></tr>
        </table>
'''
LEGENDS = LEGENDS % (COLOR_EQ, 'Strings are equal',
                    COLOR_NEQ, 'Strings are not equal',
                    COLOR_MISSED, 'String is missed in application output',
                    COLOR_NOT_COVERED, 'String is contained in the output but not covered')

def replate_symbols_for_html(string):
    return string.replace(" ", NON_BREAKING_SPACES).replace("<", LT).replace(">", GT)

def is_output_expected(f_output, f_expect, f_result):
    result = True
    with open(f_output, 'r') as (f_out), open(f_expect, 'r') as (f_exp):
            out = f_out.read().splitlines()
            expect = f_exp.read().splitlines()
            # Convert to map of str_eq_re
            out = [*map(str_eq_re, out)]
            expect = [*map(str_eq_re, expect)]

    len_out = len(out)
    len_expect = len(expect)
    size = min(len_out, len_expect)
    if size != len_expect:
        result = False
    rows = ''
    for i in range(size):
        lines_equal = out[i] == expect[i]
        if not lines_equal:
            result = False
        rows += ROW % (COLOR_EQ if lines_equal else COLOR_NEQ, i + 1, replate_symbols_for_html(out[i]), replate_symbols_for_html(expect[i]))

    for i in range(size, len_expect):
        rows += ROW % (COLOR_MISSED, i + 1, '', expect[i])

    for i in range(max(size, len_expect), len_out):
        rows += ROW % (COLOR_NOT_COVERED, i + 1, out[i], '')

    if len_out == len_expect:
        files_len_color = COLOR_EQ
    elif len_out < len_expect:
        files_len_color = COLOR_MISSED
    elif len_out > len_expect:
        files_len_color = COLOR_NOT_COVERED
    len_compare = FILES_LEN % (files_len_color, len_out + 1, len_expect + 1)
    page = TABLE % (f_output, f_expect, rows, len_compare, LEGENDS)

    os.makedirs(os.path.dirname(os.path.realpath(f_result)), exist_ok=True)
    with open(f_result, 'w') as (f_res):
        f_res.write(page)

    return result
