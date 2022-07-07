#!/bin/bash

# This scripts tests influence of the cumulative count
DIR=cumulative_testing
rm -rf $DIR
mkdir $DIR
mkdir $DIR/meta-mapi
mkdir $DIR/meta-itt

function run() {
    for CNT in 1 10 100 1000; do
        echo --- $1 $CNT ---
        itt_benchmark  -m $1 -i 1000000 -c $CNT -u ns -p $DIR/$1/dump_c$CNT
        sleep 3
    done
}

run meta-mapi
run meta-itt