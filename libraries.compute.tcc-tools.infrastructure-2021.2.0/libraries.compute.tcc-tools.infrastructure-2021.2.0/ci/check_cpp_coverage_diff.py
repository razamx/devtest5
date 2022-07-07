#!/usr/bin/env python3
# /*******************************************************************************
# INTEL CONFIDENTIAL
# Copyright 2020-2021 Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

import sys
import argparse
from xml.dom import minidom
from dataclasses import dataclass


@dataclass
class covered_file_attributes:
    line_count: int
    line_coverage: float
    branch_coverage: float


class parser:
    def __read_xml(self, coverage_report):
        COVERAGE_TAG = "coverage"
        return minidom.parse(coverage_report).getElementsByTagName(COVERAGE_TAG)[0]

    def __get_line_coverage(self, coverage_tree):
        LINE_RATE_ATTRIBURE_NAME = "line-rate"
        return float("{:.3f}".format(float(coverage_tree.getAttribute(LINE_RATE_ATTRIBURE_NAME))))

    def __get_branch_coverage(self, coverage_tree):
        BRANCH_RATE_ATTRIBURE_NAME = "branch-rate"
        return float("{:.3f}".format(float(coverage_tree.getAttribute(BRANCH_RATE_ATTRIBURE_NAME))))

    def __get_line_count(self, coverage_tree):
        LINES_TAG = "lines"
        LINE_TAG = "line"
        LINE_NUMBER_TAG = "number"
        return int(coverage_tree.getElementsByTagName(LINES_TAG)[0].getElementsByTagName(LINE_TAG)[-1].getAttribute(LINE_NUMBER_TAG))

    def __get_covered_modules(self, coverage_tree):
        COVERED_MODULES_TAG = "packages"
        COVERED_MODULE_TAG = "package"
        return coverage_tree.getElementsByTagName(COVERED_MODULES_TAG)[0].getElementsByTagName(COVERED_MODULE_TAG)

    def __get_covered_files_from_module(self, module_tree):
        COVERED_CLASSES_TAG = "classes"
        COVERED_FILES_TAG = "class"
        return module_tree.getElementsByTagName(COVERED_CLASSES_TAG)[0].getElementsByTagName(COVERED_FILES_TAG)

    def __get_filename_for_covered_file(self, file_tree):
        FILENAME_ATTRIBURE_NAME = "filename"
        return file_tree.getAttribute(FILENAME_ATTRIBURE_NAME)

    def parse_coverage_file(self, coverage_xml):
        total_coverage = ()
        coverage_per_file = {}

        tree = self.__read_xml(coverage_xml)
        total_coverage = (self.__get_line_coverage(tree), self.__get_branch_coverage(tree))

        covered_modules = self.__get_covered_modules(tree)
        for module in covered_modules:
            covered_files = self.__get_covered_files_from_module(module)
            for file in covered_files:
                filename = self.__get_filename_for_covered_file(file)
                attributes = covered_file_attributes(
                    line_count=self.__get_line_count(file),
                    line_coverage=self.__get_line_coverage(file),
                    branch_coverage=self.__get_branch_coverage(file)
                )
                coverage_per_file.update({filename: attributes})
        return total_coverage, coverage_per_file


def build_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--main_report", required=True, type=str, dest="main_report")
    parser.add_argument("--current_report", required=True, type=str, dest="current_report")
    parser.add_argument("--min_line_coverage", type=float, dest="min_line_coverage", default=0.8)
    parser.add_argument("--min_branch_coverage", type=float, dest="min_branch_coverage", default=0.7)
    parser.add_argument("--changed_files", type=str, dest="changed_files", default=[], nargs="+")
    return parser


def compare_coverage_reports(main_report, current_report, changed_files, minimal_total_line_coverage, minimal_branch_line_coverage):
    """
        Changed files contains modified files and renamed files.

        not_covered_new_files = curr_coverage & not main_coverage & not changed_files and coverage less than minimal
        redused_coverage = curr_coverage & main_coverage & changed_files and coverage less than master
        edited_not_covered_files = curr_coverage & changed_files and coverage equals zero
    """

    redused_coverage = []
    not_covered_new_files = []
    edited_not_covered_files = []
    report_parser = parser()
    main_total_coverage, main_coverage = report_parser.parse_coverage_file(main_report)
    curr_total_coverage, curr_coverage = report_parser.parse_coverage_file(current_report)

    if curr_total_coverage[0] >= minimal_total_line_coverage and curr_total_coverage[1] >= minimal_branch_line_coverage:
        return redused_coverage, not_covered_new_files, edited_not_covered_files

    for filename in curr_coverage:
        curr_attributes = curr_coverage[filename]
        curr_line_coverage, curr_branch_coverage = curr_attributes.line_coverage, curr_attributes.branch_coverage

        if filename in main_coverage:
            main_attributes = main_coverage[filename]
            main_line_coverage, main_branch_coverage = main_attributes.line_coverage, main_attributes.branch_coverage

            if filename in changed_files:
                curr_line_coverage_less = main_line_coverage > curr_line_coverage
                curr_branch_coverage_less = main_branch_coverage > curr_branch_coverage
                if curr_line_coverage_less or curr_branch_coverage_less:
                    redused_coverage.append(filename)

                uncovered_file = curr_line_coverage == 0.0
                if uncovered_file:
                    edited_not_covered_files.append(filename)
        else:
            if filename in changed_files:
                uncovered_file = curr_line_coverage == 0.0
                if uncovered_file:
                    edited_not_covered_files.append(filename)
            else:
                curr_line_coverage_less = minimal_total_line_coverage > curr_line_coverage
                curr_branch_coverage_less = minimal_branch_line_coverage > curr_branch_coverage
                if curr_line_coverage_less or curr_branch_coverage_less:
                    not_covered_new_files.append(filename)

    return redused_coverage, not_covered_new_files, edited_not_covered_files


def print_report(redused_coverage, not_covered_new_files, edited_not_covered_files, minimal_total_line_coverage, minimal_branch_line_coverage):
    for file in redused_coverage:
        print(f"The file coverage has decreased: {file}")
    for file in not_covered_new_files:
        print(f"The new file has line coverage < {minimal_total_line_coverage} and branch coverage < {minimal_branch_line_coverage}: {file}")
    for file in edited_not_covered_files:
        print(f"Edited uncovered file detected, please write test for this file: {file}")


def main(args):
    return_code = 0
    changed_files = [f"tccsdk/{file}" for file in args.changed_files]
    redused_coverage, not_covered_new_files, edited_not_covered_files = compare_coverage_reports(args.main_report, args.current_report, changed_files, args.min_line_coverage, args.min_branch_coverage)
    if redused_coverage or not_covered_new_files or edited_not_covered_files:
        print_report(redused_coverage, not_covered_new_files, edited_not_covered_files, args.min_line_coverage, args.min_branch_coverage)
        return_code = 1
    sys.exit(return_code)


if __name__ == "__main__":
    args = build_parser().parse_args()
    main(args)
