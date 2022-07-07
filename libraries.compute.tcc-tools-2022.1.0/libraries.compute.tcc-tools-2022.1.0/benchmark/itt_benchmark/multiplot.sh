#!/bin/bash
LARGE_MAX=$2
PLOTS_DIR=./plots

function help() {
	echo "Plots multiple measurements together and separated with different scale"
	echo "Usage:"
	echo '	multiplot.sh -f <fild wildcard> -l <large scale up bound> -s <small scale up bound>'
}

while getopts "h:f:l:s:" opt
do
case $opt in
	h) help ;;
	f) FILES=$OPTARG;;
	l) LARGE=$OPTARG;;
	s) SMALL=$OPTARG;;
	*)
		echo "Invalid option"
		exit 1 
		;;
esac
done

if [ -z "$FILES" ]; then
	echo "Must specify files"
	exit 1
fi

rm -rf $PLOTS_DIR
mkdir $PLOTS_DIR

if [ ! -z $LARGE ]; then
	LS_ARG="--tmax $LARGE"
	log_view.py --tx $FILES --tmin 0 $LS_ARG --output $PLOTS_DIR/plot_large_scale_all.png > statistics.txt
	for FILE in $FILES; do 
		log_view.py --tx $FILE --tmin 0 $LS_ARG --output $PLOTS_DIR/plot_large_scale_$FILE.png
	done
fi

if [ ! -z $SMALL ]; then
	SS_ARG="--tmax $SMALL"
	log_view.py --tx $FILES --tmin 0 $SS_ARG --output $PLOTS_DIR/plot_small_scale_all.png
	for FILE in $FILES; do 
		log_view.py --tx $FILE --tmin 0 $SS_ARG --output $PLOTS_DIR/plot_small_scale_$FILE.png
	done
fi