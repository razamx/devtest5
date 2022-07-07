#!/bin/bash

FILES_TO_CHECK=(
	/usr/include/tcc
	/usr/lib64/libtcc*
	/usr/bin/tcc_*
	/usr/share/tcc_tools
	/home/root/.tcc_tools/install-info.cfg
	/usr/lib64/libinih.so*
	/usr/lib64/libumm_malloc.so*
	/usr/bin/opcua_server
)

usage() {
	echo "remove_tcc_files.sh - Check that TCC Tools is uninstalled and do cleanup"
	echo "Usage":
	echo "remove_tcc_files.sh [-d]"
	echo "-d   delete all files"
}


delete() {
	for line in "${FILES_TO_CHECK[@]}"; do
		rm -rf $line
	done
}

check() {
	result=0
	for line in "${FILES_TO_CHECK[@]}"; do
		for f in ${line} ; do
			if [ -e "${f}" ]; then
				1>&2 echo "TCC file was not removed: ${f}"
				result=1
			fi
		done
	done

	if [ ${result} != 0 ]; then
		for log in /home/root/.tcc_tools/*_info.log; do
			echo "Log: ${log}\n\n"
			# Temporary disabled course log is too long
			# Maybe we should copy it?
			#        cat ${log}
		done
		if [[ "$DELETE" == 1 ]]; then
			echo "Delete remain files"
			delete
		fi
	fi

	exit $result
}

while getopts "dh" opt
do
case $opt in
d) DELETE=1;;
*)
	usage
	exit 0
;;
esac
done

check
