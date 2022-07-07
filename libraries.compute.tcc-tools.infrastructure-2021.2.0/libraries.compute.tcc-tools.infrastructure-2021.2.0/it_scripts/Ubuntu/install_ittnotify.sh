#!/bin/bash
set -e

#################################################
### Install ittnotify lib and headers to      ###
### INSTALL_PATH_LIB and INSTALL_PATH_INCLUDE ###
### for lib and headers respectively          ###
#################################################

if [ -z "${INSTALL_PATH_LIB}" ]; then
	INSTALL_PATH_LIB="/usr/lib/"
fi

if [ -z "${INSTALL_PATH_INCLUDE}" ]; then
	INSTALL_PATH_INCLUDE="/usr/include/"
fi

# clone IntelSEAPI repo
rm -rf IntelSEAPI
git clone --single-branch https://github.com/intel/IntelSEAPI
cd IntelSEAPI
cd ittnotify

# lib
cmake -DARCH_64=1
make

echo "Copy libittnotify64.a to ${INSTALL_PATH_LIB}"
cp libittnotify64.a  ${INSTALL_PATH_LIB}

# headers
echo "Copy headers to ${INSTALL_PATH_INCLUDE}"
cp -r include ${INSTALL_PATH_INCLUDE}/ittnotify
cp -r src/ittnotify/*h ${INSTALL_PATH_INCLUDE}/ittnotify

# Delete repo
cd ../../
rm -fr IntelSEAPI

echo "Done"
