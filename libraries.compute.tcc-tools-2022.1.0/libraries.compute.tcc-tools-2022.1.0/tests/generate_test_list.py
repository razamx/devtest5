#!/usr/bin/env python3
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

'''
The script generate list of Robot Framework suites that compatible for workflow on current platform
'''

import os
import sys
import argparse
import ast
from robot.api import get_model, Token
from robot.api.parsing import ModelVisitor


def parse_args():
    # create parser:
    parser = argparse.ArgumentParser(
        add_help=True,
        formatter_class=argparse.RawTextHelpFormatter
    )

    subparsers = parser.add_subparsers(
        help='Supporter commands',
        dest='type'
    )

    # host command:
    parser_host = subparsers.add_parser(
        'host',
        help='tests which runs only on host',
        formatter_class=argparse.RawTextHelpFormatter
    )

    # target command:
    parser_target = subparsers.add_parser(
        'target',
        help='tests which runs only on target',
        formatter_class=argparse.RawTextHelpFormatter
    )

    # host_target command:
    parser_host_target = subparsers.add_parser(
        'host_target',
        help='tests which runs on host with target connected',
        formatter_class=argparse.RawTextHelpFormatter
    )

    # two_targets command:
    parser_two_targets = subparsers.add_parser(
        'two_targets',
        help='tests which run on a pair of connected targets',
        formatter_class=argparse.RawTextHelpFormatter
    )

    # required arguments for all:
    for name, subparser in subparsers.choices.items():
        subparser.add_argument('--input_file',
                               required=True,
                               help='List with tests suites')
        subparser.add_argument('--output_file',
                               required=True,
                               help='Output file for list of suites')
        subparser.add_argument('--workflow',
                               choices=['BKC', 'PRE_COMMIT', 'DAILY', 'COMPATIBLE', 'PV'],
                               required=True,
                               help='Specify test workflow:\n'
                                    '  BKC:         tests to be delivered to the BKC\n'
                                    '  PRE_COMMIT:  tests to be run in pre-commit builder\n'
                                    '  DAILY:       tests to be run in daily builder\n'
                                    '  COMPATIBLE:  any tests compatible with platform\n'
                                    '  PV:          tests to be run in oneCI')
        if name != 'host':
            subparser.add_argument(
                '--platform_name',
                required=True,
                help='Specify platform name. Will be added all test that compatible with ALL or specified platform')

    parser.usage = '%(prog)s host | target | host_target | two_targets'
    parser_host.usage = '%(prog)s --input_file <input_file> --workflow <workflow> --output_file <output_file>'
    parser_target.usage = '%(prog)s --input_file <input_file> --platform_name <platform_name> --workflow <workflow> --output_file <output_file>'
    parser_host_target.usage = '%(prog)s --input_file <input_file> --platform_name <platform_name> --workflow <workflow> --output_file <output_file>'
    parser_two_targets.usage = '%(prog)s --input_file <input_file> --platform_name <platform_name> --workflow <workflow> --output_file <output_file>'
    return parser.parse_args()


class TestNamePrinter(ModelVisitor):
    result = dict()
    def visit_File(self, node):
        self.result = {Token.DEFAULT_TAGS: [], Token.FORCE_TAGS: [], Token.TAGS: {}}
        for section in node.sections:
            for tokens in section.body:
                try:
                    if tokens.type == Token.FORCE_TAGS or tokens.type == Token.DEFAULT_TAGS:
                        values = list(filter(None, tokens.values))
                        self.result[tokens.type] = values
                except AttributeError:
                    pass
        self.generic_visit(node)

    def visit_TestCase(self, node):
        test_name_lineno = f"{node.name} {node.lineno}"
        self.result[Token.TAGS][test_name_lineno] = list()
        for statement in node.body:
            try:
                if statement.type == Token.TAGS:
                    self.result[Token.TAGS][test_name_lineno] = list(statement.values)
            except AttributeError:
                pass


# Check is suite contain any test with all necessary tags and not contain unexpected tags
def is_compatible(suite, expected_tags, unexpected_tags, platform_tags=None):
    force_tags = []
    default_tags = []

    if len(suite[Token.FORCE_TAGS]) > 0:
        force_tags.extend(suite[Token.FORCE_TAGS])

    if len(suite[Token.DEFAULT_TAGS]) > 0:
        default_tags.extend(suite[Token.DEFAULT_TAGS])

    for test in suite[Token.TAGS]:

        case_tags = force_tags.copy()

        # If the test does not contain any tags the default tags will be used
        # If the test contains some tag the default tags will be ignored
        # For more information read RF documentation:
        # https://robotframework.org/robotframework/latest/RobotFrameworkUserGuide.html#tagging-test-cases

        if len(suite[Token.TAGS][test]) > 0:
            case_tags.extend(suite[Token.TAGS][test])
        else:
            case_tags.extend(default_tags)

        # Tags in robot are case independent
        case_tags = [x.lower() for x in case_tags]
        expected_match = all(expected_tag in case_tags for expected_tag in expected_tags)
        unexpected_match = not any(unexpected_tag in case_tags for unexpected_tag in unexpected_tags)

        platform_match = True
        if platform_tags:
            platform_match = any(platform in case_tags for platform in platform_tags)

        if expected_match and unexpected_match and platform_match:
            return True
    return False    


def main():
    args = parse_args()

    suite_set = set()
    platform_tags = []
    if args.type != 'host':
        platform_tags = ['ALL', args.platform_name]
    expected_tags = []
    unexpected_tags = []

    if args.workflow == 'BKC':
        expected_tags.extend(['BKC'])
    elif args.workflow == 'PRE_COMMIT':
        unexpected_tags.extend(['NO_PRE_COMMIT'])
    elif args.workflow == 'DAILY':
        unexpected_tags.extend(['NO_DAILY'])
    elif args.workflow == 'PV':
        expected_tags.extend(['PV'])

    if args.type == 'host':
        expected_tags.extend(['HOST'])
        unexpected_tags.extend(['TWO_TARGETS'])
    elif args.type == 'target':
        unexpected_tags.extend(['HOST', 'HOST_TARGET', 'TWO_TARGETS'])
    elif args.type == 'host_target':
        expected_tags.extend(['HOST_TARGET'])
        unexpected_tags.extend(['TWO_TARGETS'])
    elif args.type == 'two_targets':
        expected_tags.extend(['TWO_TARGETS'])

    # Tags in robot are case independent
    expected_tags = [x.lower() for x in expected_tags]
    unexpected_tags = [x.lower() for x in unexpected_tags]
    platform_tags = [x.lower() for x in platform_tags]

    with open(args.input_file) as f:
        suites = [line.rstrip() for line in f]
        for suite_path in suites:
            suite = get_model(suite_path)
            printer = TestNamePrinter()
            printer.visit(suite)
            if is_compatible(printer.result, expected_tags, unexpected_tags, platform_tags):
                suite_set.add(suite_path)

    if len(suite_set) > 0:
        os.makedirs(os.path.dirname(args.output_file), exist_ok=True)

        with open(args.output_file, 'w') as f:
            for item in suite_set:
                f.write(item + "\n")


if __name__ == "__main__":
    sys.exit(main())
