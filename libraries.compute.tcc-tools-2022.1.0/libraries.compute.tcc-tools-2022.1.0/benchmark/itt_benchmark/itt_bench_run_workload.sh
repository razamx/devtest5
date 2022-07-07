#!/bin/bash

# This script runs workload and measure it with MAPI and ITT
DIR=wokrload_testing
rm -rf $DIR
mkdir $DIR

function run() {
    for CNT in 1 10 100 1000 10000; do
        echo --- $1 $CNT ---
        itt_benchmark  -m $1 -i 1000000 -w $CNT -u ns -p $DIR/dump_w$CNT
        sleep 3
    done
}

run workload-mapi
run workload-itt