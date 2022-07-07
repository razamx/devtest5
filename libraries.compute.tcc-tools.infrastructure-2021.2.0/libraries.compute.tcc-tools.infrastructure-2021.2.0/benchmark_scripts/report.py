#!/usr/bin/env python3

import os
import sys
import re
import argparse

import itertools
import functools
import json
import numpy as np
import matplotlib.pyplot as plt


def create_parser() -> argparse.ArgumentParser:
    """
    Create argument parser
    :return: argument parser
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('folder',
                        help='location of results')
    return parser


def extract_from_run(directory):
    """
    Extract data from one experiment specified by directory
    :param directory: place where is resulting files is located
    :return: dictionary of experiments data
    """
    content = os.listdir(directory)
    required_files = ["out.txt", "dump.txt", "arguments.json"]
    if functools.reduce(lambda a, x: a or (x not in content), required_files, False):
        raise Exception("Required files were not found")

    def load_from_dump(file):
        """
        Load probes data from
        :param file: file content in string
        :return: dictionary: measurer_name -> [probes]
        """
        regex_extract = re.compile(r"([^:]*): (.*)")
        search_result = re.findall(regex_extract, file)
        return dict(map(lambda x: (x[0], [int(y) for y in x[1].split(',') if y.strip().isdigit()]), search_result))

    def load_from_out(file):
        """
        Load statistics from files
        :param file:  file content in string
        :return: dictionary: statistic -> value_of_statistic
        """
        regex_metrics = re.compile(r"avg=(\d+\.?\d*) min=(\d+\.?\d*) max=(\d+\.?\d*) jitter=(\d+\.?\d*)")
        search_result = re.search(regex_metrics, file).groups()
        return dict(zip(["avg", "min", "max", "jitter"], map(float, search_result)))

    def load_from_perf(file):
        """
        Load misses/hits from files
        :param file:  file content in string
        :return: dictionary of hit misses
        """
        regex_extract_hit = re.compile(r"(\d+\.?\d*).*l(\d)_hit")
        regex_extract_miss = re.compile(r"(\d+\.?\d*).*miss")
        hit, level = map(int, re.search(regex_extract_hit, file).groups())
        miss = int(re.search(regex_extract_miss, file).groups()[0])
        return {"hit": hit, "miss": miss, "cache_level": level}

    with open(os.path.join(directory, "out.txt")) as f:
        out = load_from_out(f.read())
    with open(os.path.join(directory, "dump.txt")) as f:
        dump = load_from_dump(f.read())
    with open(os.path.join(directory, "arguments.json")) as f:
        arguments = json.load(f)
    perf = {}
    if "perf.txt" in os.listdir(directory):
        with open(os.path.join(directory, "perf.txt")) as f:
            perf = load_from_perf(f.read())   
    else:
        regex_extract_level = re.compile(r".*l(\d)_hit")
        level = int(regex_extract_level.search(directory).group(1))
        perf = {"hit": 0, "miss": 0, "cache_level": level}
    return {
        "arguments": arguments,
        "out": out,
        "dump": dump,
        "perf": perf
    }


def common_bins(extracted_data):
    """
    Extract common bins for data to allow comparing of plots
    :param extracted_data: dictionary of extracted data
    :return: list of bins
    """
    min_elem = None
    max_elem = None

    def relaxation(func, prev, collection):
        """
        Utility function to allow relaxation when previous value can be undefined
        :param func: relaxation function
        :param prev: previous value
        :param collection: collection to relax
        :return: relaxed value
        """
        if prev is None:
            return func(collection)
        else:
            return func(prev, func(collection))

    for pack in extracted_data:
        for experiment in pack:
            dump = experiment["probes"]
            min_elem, max_elem = relaxation(min, min_elem, dump), relaxation(max, max_elem, dump)

    return np.logspace(np.log10(min_elem - 1), np.log10(max_elem + 1), num=100)


def unite_caches(data):
    """
    Utility function to unite data for different level cache experiments
    :param data: extracted data from files
    :return: Half-processed data where caches probes was united
    """
    l1 = {k: v for (k, v) in data.items() if v["perf"]["cache_level"] == 1}
    l2 = {k: v for (k, v) in data.items() if v["perf"]["cache_level"] == 2}

    result = []
    for k1, v1 in l1.items():
        probes_num = 4096 if v1["arguments"][0]["random"] is False else 1024
        for k2, v2 in l2.items():
            if v1["arguments"][0] == v2["arguments"][0]:
                #TODO remove hardcode on Workload name
                if v1["arguments"][0]["cumulative"] == False:
                    result.append({"probes": list(v1["dump"]["Workload"] + v2["dump"]["Workload"]),
                                   "perf": {"L1": v1["perf"], "L2": v2["perf"]},
                                   "out": {"L1": v1["out"], "L2": v2["out"]},
                                   "arguments": v1["arguments"][0]})

                else:
                    result.append({"probes": list(map(lambda x: x / probes_num, v1["dump"]["Workload"] + v2["dump"]["Workload"])),
                                   "perf": {"L1": v1["perf"], "L2": v2["perf"]},
                                   "out": {"L1": v1["out"], "L2": v2["out"]},
                                   "arguments": v1["arguments"][0]})
    return result


def process_data(extracted_data):
    """
    Function to process extracted data from files
    :param extracted_data: extracted data from files
    :return: rearranged data grouped in tuples by lock parameter
    """
    lock = {k: v for (k, v) in extracted_data.items() if v["arguments"][0]["lock"] is True}
    nolock = {k: v for (k, v) in extracted_data.items() if v["arguments"][0]["lock"] is False}

    lock_united = unite_caches(lock)
    nolock_united = unite_caches(nolock)

    result = []
    for l, nl in itertools.product(lock_united, nolock_united):
        l_f = dict((k, v) for k, v in l["arguments"].items() if k != "lock")
        nl_f = dict((k, v) for k, v in nl["arguments"].items() if k != "lock")
        if l_f != nl_f:
            continue
        result.append((l, nl))

    return result


def process_results_for_cache_lock(directory):
    """
    Main function to process results specified by experiments folder
    :param directory: folder where results file from experiment is located
    """
    folders = os.listdir(directory)

    extracted_data = {}
    for folder in folders:
        current_path = os.path.join(directory, folder)
        extracted_data[current_path] = extract_from_run(current_path)

    processed_data = process_data(extracted_data)

    bins = common_bins(processed_data)

    def generate_title(arguments):
        return " ".join(map(lambda x: "{0}: {1};".format(*x), arguments.items()))

    def generate_filename(arguments):
        return "_".join(map(lambda x: "{0}-{1}".format(*x), arguments.items())) + ".png"

    TIME_UNITS = {"0" : "Ticks", "1" : "Nanoseconds", "2" : "Microseconds"}
    for pack in processed_data:
        lock, nolock = pack
        fig1, ax1 = plt.subplots()
        plt.title(generate_title(dict((k, v) for k, v in lock["arguments"].items() if k != "lock" and k != "time_units")))
        ax1.hist(lock["probes"], bins=bins, alpha=0.5, label="With pseudo-lock", edgecolor='grey')
        ax1.hist(nolock["probes"], bins=bins, alpha=0.5, label="Without pseudo-lock", edgecolor='grey')
        ax1.set_xlabel(TIME_UNITS[str(list(v for k, v in lock["arguments"].items() if k == "time_units")[0])])
        ax1.set_ylabel("Times occurred")
        ax1.set_xscale('log')
        ax1.set_yscale('log', nonposy='clip')
        ax1.legend(loc='upper right')
        #ax1.set_xticks([10000, 20000, 30000, 100000, 200000])
        #ax1.get_xaxis().set_major_formatter(matplotlib.ticker.FormatStrFormatter('%.E'))
        ax1.text(0.62,
                 0.75,
                 "L1 - miss: {0}, hit: {1}\nL2 - miss: {2}, hit: {3}".format(
                     *map(str, [lock["perf"]["L1"]["miss"],
                                lock["perf"]["L1"]["hit"],
                                lock["perf"]["L2"]["miss"],
                                lock["perf"]["L2"]["hit"]])),
                 fontsize=10,
                 transform=plt.gcf().transFigure,
                 horizontalalignment='left',
                 color='#1f77b4')
        ax1.text(0.62,
                 0.68,
                 "L1 - miss: {0}, hit: {1}\nL2 - miss: {2}, hit: {3}".format(
                     *map(str, [nolock["perf"]["L1"]["miss"],
                                nolock["perf"]["L1"]["hit"],
                                nolock["perf"]["L2"]["miss"],
                                nolock["perf"]["L2"]["hit"]])),
                 fontsize=10,
                 transform=plt.gcf().transFigure,
                 horizontalalignment='left',
                 color='#ff7f0e')
        fig1.set_size_inches(10, 6)
        fig1.set_tight_layout(True)
        plt.savefig(generate_filename(dict((k, v) for k, v in lock["arguments"].items() if k != "lock")))
        #plt.show()

def main():
    parser = create_parser()
    args = parser.parse_args()
    process_results_for_cache_lock(args.folder)
    return 0


if __name__ == "__main__":
    sys.exit(main())
