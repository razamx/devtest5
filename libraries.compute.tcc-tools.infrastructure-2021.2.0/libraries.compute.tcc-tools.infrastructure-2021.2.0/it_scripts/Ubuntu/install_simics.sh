#!/bin/bash
set -e

SIMICS_SHARE=/nfs/inn/disks/tcc_incoming/simics
TMP_FOLDER=/tmp/simics_installation
SIMICS_INSTALL_FOLDER=/opt/simics/simics-5/
BASE_FOLDER=/opt/simics/base
IFWI_FOLDER_NAME='EHL IFWI'
IMAGE_NAME=core-image-sato-sdk-intel-corei7-64*.wic
ACCOUNT_NAME=NULL

if [ "${EUID}" -ne 0 ]; then
    echo "Please run as root"
    exit
fi

echo "WARNING: folder /opt/simics with simics base and simics-project will be deleted"
echo "Type [y] or n:"
read KEYBOARD
case $KEYBOARD in
"")
    echo "Removing folder"
    ;;
"y")
    echo "Removing folder"
    ;;
"n")
    echo "Interrupted"
    exit 1
    ;;
*)
    echo "Interrupted"
    exit 1
    ;;
esac

cd ${SIMICS_SHARE}
echo "Type one of folow Simics version:"

for folder in $(ls -d */); do
    echo ${folder}
done

read SELECTED_SIMICS_VERSION
echo "Started installation of ${SELECTED_SIMICS_VERSION}"

# Copy simics files to tmp directory
rm -rf ${TMP_FOLDER}
mkdir -p ${TMP_FOLDER}
rsync -ah --progress ${SIMICS_SHARE}/${SELECTED_SIMICS_VERSION}/* ${TMP_FOLDER}/

# Remove old simics folder
rm -rf /opt/simics/

# Install simics
apt install smbclient

chmod -R 777 ${TMP_FOLDER}
cd ${TMP_FOLDER}

echo "Enter you Windows account login"
read ACCOUNT_NAME
sudo -u ${ACCOUNT_NAME} bash -c 'for arch in *linux64.tar; do tar -xvf ${arch}; done'
cd simics-5-install/
sudo -u ${ACCOUNT_NAME} bash -c "./install-simics.pl -p ${SIMICS_INSTALL_FOLDER} -a"
cd ..
cp extra-lib-ubuntu/* ${SIMICS_INSTALL_FOLDER}/simics-hasf*/has_wx3/lib64

# Copy project files
mkdir -p ${BASE_FOLDER}
cp -r "${IFWI_FOLDER_NAME}" ${BASE_FOLDER}
echo "Unpacking..."
bzip2 -d ${IMAGE_NAME}.bz2
mv ${IMAGE_NAME} ${BASE_FOLDER}
mv *_rom.bin ${BASE_FOLDER}
mv simics-project ${BASE_FOLDER}

echo "Installation done"
echo "To start simics you may type follow command:"
echo "sudo /opt/simics/simics-5/simics-5.<version>/bin/simics /opt/simics/simics-project/targets/x86-<target>/<target>.simics"
echo "For example:"
echo "sudo /opt/simics/simics-5/simics-5.0.184/bin/simics /opt/simics/simics-project/targets/x86-ehl/ehl.simics"
