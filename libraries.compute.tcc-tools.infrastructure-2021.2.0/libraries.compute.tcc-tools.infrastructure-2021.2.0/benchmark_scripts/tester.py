#!/usr/bin/env python3

import sys
import os

import itertools
import functools
import datetime

import subprocess
import argparse
import json


def json_from_file(filename):
    """
    Read json file as python object
    :param filename: path to the file
    :return: python object representation of json
    """
    with open(filename) as f:
        result = json.load(f)
    return result


def create_parser() -> argparse.ArgumentParser:
    """
    Create argument parser
    :return: argument parser
    """
    parser = argparse.ArgumentParser(description='Run cache_lock_benchmark in massive testing')
    parser.add_argument('config',
                        type=json_from_file,
                        help='configuration file in yaml format')
    parser.add_argument('output_folder',
                        help='output folder to store results of testing')
    parser.add_argument(
        '-perf',        
        help='allows to run application with perf\n',
        action='store_true'
    )
    return parser


def args_set_to_list(args_set, arg_provider):
    """
    linearize dictionary of arguments to list
    :param args_set: dictionary of arguments
    :param arg_provider: function to convert argument to string representation
    :return: list of arguments
    """
    return functools.reduce(lambda a, x: a + arg_provider(*x),
                            args_set.items(),
                            [])


def process_tool_args(raw_args):
    """
    Convert input dictionary of arguments values to list of dictionaries where each dictionary is
    element of Cartesian product of each argument value sets
    :param raw_args: dictionary of arguments
    :return: list of dictionaries where each dictionary is complete argument set for program run
    """
    def append_for_each(e, l):
        """
        Return list of tuples where each tuple of size two and first element is e where the second taken from l
        :param e: first element in tuple to append
        :param l: list of seconds tuple elements
        :return: list of tuples
        """
        return list(map(lambda v: (e, v), l))
    return list(map(dict, itertools.product(*map(lambda item: append_for_each(*item), raw_args.items()))))


def create_experiment_name(benchmark_args, perf_events):
    """
    Create experiment name based on arguments set
    :param benchmark_args: dictionary of arguments for benchmark
    :param perf_events: perf arguments
    :return: string representing experiment
    """
    elements = map(lambda x: "{0}-{1}".format(*x), list(benchmark_args.items()))
    return "_".join(list(elements) + perf_events)


def benchmark_arg_provider(arg, val):
    """
    Utility function to use simpler syntax in configuration file
    :param arg: argument key
    :param val: value to provide
    :return: list of size 2
    """
    if val is False:
        return ["--no{0}".format(arg)]
    elif val is True:
        return ["--{0}".format(arg)]
    else:
        return ["--{0}".format(arg), str(val)]


def run_experiments(benchmark, experiments, output_folder, perf_flag=False):
    """
    Main function to run experiment
    :param benchmark: location of benchmark
    :param experiments: list of experiments to perform
    :param output_folder: output folder where all resulting files should be stored
    """
    if not os.path.isdir(output_folder):
        raise Exception('Output folder does not exist')

    sub_folder_name = datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    sub_folder_path = os.path.join(output_folder, sub_folder_name)
    os.makedirs(sub_folder_path)
    for experiment in experiments:
        benchmark_arg, perf_events = experiment
        folder_name = create_experiment_name(benchmark_arg, perf_events)
        current_folder = os.path.join(sub_folder_path, folder_name)
        os.makedirs(current_folder)
        with open(os.path.join(current_folder, "out.txt"), "w") as f:
            os.environ["TCC_MEASUREMENTS_BUFFERS"]="Workload:10000"
            os.environ["TCC_MEASUREMENTS_DUMP_FILE"]=os.path.join(current_folder, "dump.txt")
            if perf_flag:
                print(["perf", "stat", "-e", ','.join(perf_events),
                                            benchmark,
                                            *args_set_to_list(benchmark_arg, benchmark_arg_provider)])
                process = subprocess.Popen(["perf", "stat", "-e", ','.join(perf_events),
                                            "-o", os.path.join(current_folder, "perf.txt"),
                                            benchmark,
                                            *args_set_to_list(benchmark_arg, benchmark_arg_provider)],
                                           stdout=f,
                                           stderr=subprocess.STDOUT)
            else:
                print([benchmark, *args_set_to_list(benchmark_arg, benchmark_arg_provider)])
                process = subprocess.Popen([ benchmark,
                                            *args_set_to_list(benchmark_arg, benchmark_arg_provider)],
                                           stdout=f,
                                           stderr=subprocess.STDOUT)
            process.communicate()
        with open(os.path.join(current_folder, "arguments.json"), "w") as f:
            json.dump(experiment, f)


def main():
    argument_parser = create_parser()
    args = argument_parser.parse_args()
    benchmark_args_raw = args.config["cache_lock_benchmark_args"]
    benchmark_location = args.config["cache_lock_benchmark_location"]
    perf_events = args.config["perf_events"]

    benchmark_args = process_tool_args(benchmark_args_raw)

    experiments = list(itertools.product(benchmark_args, perf_events))
    run_experiments(benchmark_location, experiments, args.output_folder, args.perf)


if __name__ == "__main__":
    sys.exit(main())
