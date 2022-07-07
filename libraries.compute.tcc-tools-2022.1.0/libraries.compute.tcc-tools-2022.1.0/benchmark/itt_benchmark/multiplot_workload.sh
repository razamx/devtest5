#!/bin/bash
PLOTS_DIR=./plots

function help() {
	echo "Plots multiple workload measurements"
	echo "Usage:"
	echo '	multiplot.sh -m <max>'
}

while getopts "h:m:" opt
do
case $opt in
	h) help ;;
	m) MAX=$OPTARG;;
	*)
		echo "Invalid option"
		exit 1
		;;
esac
done

if [ -z "$MAX" ]; then
	echo "Must specify max"
	exit 1
fi


rm -rf $PLOTS_DIR
mkdir $PLOTS_DIR

COUNTS=$(ls | grep dump | sed 's/^dump_w\([0-9]*\)_.*$/\1/' | sort -u)

for CNT in $COUNTS; do
    log_view.py --tx dump_w${CNT}_workload_* --tmin 0 --tmax $MAX --output $PLOTS_DIR/plot_workload_w$CNT.png
done