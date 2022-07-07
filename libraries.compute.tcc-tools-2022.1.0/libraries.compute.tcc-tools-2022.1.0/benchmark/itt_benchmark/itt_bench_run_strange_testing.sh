#!/bin/bash

# This script tests for some strange things in benchmark behaviour, such as
# difference results when running benchmark multiple times
DIR=strange_testing
rm -rf $DIR
mkdir $DIR
mkdir $DIR/empty-mapi
mkdir $DIR/empty-itt
mkdir $DIR/meta-mapi
mkdir $DIR/meta-itt

function run() {
    echo --- $1 $2 ---
    itt_benchmark  -m $1 -i 1000000 -c 100 -u ns -p $DIR/$1/dump_$2
    sleep 3
}

for I in {1..10}; do
    run empty-mapi $I
    run empty-itt $I
    run meta-mapi $I
    run meta-itt $I
done