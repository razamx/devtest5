#!/bin/bash

DIR=results
rm -rf $DIR
mkdir $DIR

function run() {
    echo --- $1 ---
    itt_benchmark  -m $1 -i 1000000 -c 100 -u ns -p $DIR/dump
    sleep 3
}

run empty-mapi
run empty-itt
run meta-mapi
run meta-itt