#!/usr/bin/env bash
# /*******************************************************************************
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

# This script installs dependencies required for Intel(R) Time Coordinated Computing Tools
# (Intel® TCC Tools) development and host tools
# Will be installed:
#   * ittapi (for applications using the measurement library and measurement library build)
#   * g++-multilib (for ittapi as build dependencies)
#   * cmake (for application build)
#   * json-c (for the cache allocation library build)
#   * python3-pip (required for the data streams optimizer and cache allocation capabilities)
#   * msgfmt (for data streams optimizer and cache configurator)
#   * checkinstall (required for libbpf and open62541 installation)
#   * python modules for samples (modules listed in ${TCC_ROOT}/samples/plot_scripts/prerequisites.txt)
#   * python modules for data streams optimizer and cache configurator (modules listed in ${TCC_TOOLS_PATH}/prerequisites.txt)
#   * Firmware and BIOS Utilities located in GitHub (for capsule creation for UEFI BIOS)
#   * Slim Bootloader tools located in GitHub (for capsule creation for Slim Bootloader)
#   * Test certificates (for data streams optimizer and cache configurator demo)
#   * python3-sphinx (required for building XDP supporting libraries)
#   * libelf library (for XDP libraries support)
#   * libbpf library (for XDP libraries support)
#   * Open62541 library (for real-time communication demo support)


#. Default settings
USER_PASS=""

#. Check command return code and exit with error in case of non-zero code
check_return_code () {
    ret=$?
    message=$1
    if [[ $ret != 0 ]]; then
        echo "ERROR: $message"
        exit $ret
    fi
}

print_pass() {
    if [[ "$-" =~ x ]]; then
        set +x
        echo "${USER_PASS}"
        set -x
    else
        echo "${USER_PASS}"
    fi
}

run_with_sudo_if_required () {
    echo "INFO: called \"$@\""

    if [ "$EUID" -ne 0 ]
        then
            eval "print_pass | sudo -S $@"

        else
            eval "$@"
    fi
}

# Update apt
function apt_update {
    run_with_sudo_if_required apt-get -y update
    check_return_code "Can't perform apt update"
}

#. Install ittapi:
function install_itt {
    ITT_BUILD_DIR=$(mktemp -d)
    cd ${ITT_BUILD_DIR}
    check_return_code "can't change working directory to ${ITT_BUILD_DIR}"

    #. Clone the ittapi repository:
    git clone -b v3.18.10 https://github.com/intel/ittapi
    check_return_code "ittapi cloning failed"

    #. Install build dependencies:
    run_with_sudo_if_required apt-get -y install cmake g++-multilib
    check_return_code "\`apt-get install cmake g++-multilib\` failed"

    #. Build itt:
    cd ittapi
    python3 buildall.py
    check_return_code "ittapi build failed"

    #. Copy itt parts to system folders:
    run_with_sudo_if_required cp build_linux/64/bin/libittnotify.a /usr/lib
    check_return_code "ittapi library copying failed"
    run_with_sudo_if_required cp -r include /usr/include/ittnotify
    check_return_code "ittapi headers copying failed"
    run_with_sudo_if_required cp src/ittnotify/*.h /usr/include/ittnotify/
    check_return_code "ittapi developer headers copying failed"
    run_with_sudo_if_required rm -rf /usr/include/ittnotify/fortran/win32
    check_return_code "ittapi removing win32 headers failed"
    run_with_sudo_if_required rm -rf /usr/include/ittnotify/fortran/posix/x86
    check_return_code "ittapi removing fortran x86 headers failed"
}

#. Install json-c:
function install_json {
    run_with_sudo_if_required apt-get -y install libjson-c-dev
    check_return_code "\`apt-get install libjson-c-dev\` failed"
}

#. Install dependencies for host-side tools:
function install_host_tools_dependencies {
    #. The dependencies are required for the data streams optimizer and cache allocation capabilities.
    run_with_sudo_if_required apt-get -y install python3-pip
    check_return_code "\`apt-get install python3-pip\` failed"

    #. Install the gettext utilities:
    run_with_sudo_if_required apt-get -y install gettext
    check_return_code "\`apt-get install gettext\` failed"

    #. Install the checkinstall package:
    run_with_sudo_if_required apt-get -y install checkinstall
    check_return_code "\`apt-get install checkinstall\` failed"

    #. Go to samples/prot_scripts directory:
    cd ${TCC_ROOT}/samples/plot_scripts
    check_return_code "cannot change working directory to ${TCC_ROOT}/samples/plot_scripts"

    #. Install the samples required Python modules for current user:
    run_with_sudo_if_required pip3 install -r prerequisites.txt
    check_return_code "installation of python3 modules for samples failed"

    #. Go to tools directory:
    cd ${TCC_TOOLS_PATH}
    check_return_code "cannot change working directory to ${TCC_TOOLS_PATH}"

    #. Install the tools required Python modules for current user:
    run_with_sudo_if_required pip3 install -r prerequisites.txt
    check_return_code "installation of python3 modules for tools failed"

    #. Install certificates:
    wget -P cert https://raw.githubusercontent.com/tianocore/edk2/master/BaseTools/Source/Python/Pkcs7Sign/TestSub.pub.pem
    check_return_code "TestSub.pub.pem certificate cloning failed"
    wget -P cert https://raw.githubusercontent.com/tianocore/edk2/master/BaseTools/Source/Python/Pkcs7Sign/TestCert.pem
    check_return_code "TestCert.pem certificate cloning failed"
    wget -P cert https://raw.githubusercontent.com/tianocore/edk2/master/BaseTools/Source/Python/Pkcs7Sign/TestRoot.pub.pem
    check_return_code "TestRoot.pub.pem certificate cloning failed"

    #. Install Firmware and BIOS Utilities
    cd ${TCC_TOOLS_PATH}/capsule
    rm -rf uefi
    check_return_code "can't remove ${TCC_TOOLS_PATH}/capsule. Remove it manually and rerun the script"
    git clone --depth 1 --branch v0.8.1 https://github.com/intel/iotg-fbu uefi
    check_return_code "FBU cloning failed"
    chmod -R a+rx uefi
    #. Install the required Python modules:
    cd ${TCC_TOOLS_PATH}/capsule/uefi
    run_with_sudo_if_required pip3 install -r requirements.txt
    check_return_code "installation of python3 modules for Firmware and BIOS Utilities"

    #. Install SBL Tools
    cd ${TCC_TOOLS_PATH}/capsule
    rm -rf sbl
    check_return_code "can't remove ${TCC_TOOLS_PATH}/capsule_sbl. Remove it manually and rerun the script"
    git clone https://github.com/slimbootloader/slimbootloader.git sbl
    check_return_code "SBL tools cloning failed"
    cd sbl 
    git checkout 8529406967b25db462d8481c7cd72fb025076747
    chmod -R a+rx ./
}

#. Generate test keys
function key_generation {
    rm -rf ${TCC_TOOLS_PATH}/keys/uefi
    mkdir -p ${TCC_TOOLS_PATH}/keys/uefi
    check_return_code "Failed to clean up keys directory."
    openssl req -new -x509 -newkey rsa:3072 -subj "/CN=Secure_b=Boot_test/" -keyout ${TCC_TOOLS_PATH}/keys/uefi/Signing.key -out ${TCC_TOOLS_PATH}/keys/uefi/Signing.crt -days 365 -nodes -sha384
    check_return_code "Key generation was failed."
}

#. Install XDP support libs
function xdp_support {
    #. The dependencies are required for building XDP supporting libraries.
    run_with_sudo_if_required apt-get -y install python3-sphinx
    check_return_code "\`apt-get install python3-sphinx\` failed"

    run_with_sudo_if_required apt-get -y install pkg-config
    check_return_code "\`apt-get install pkg-config\` failed"

    run_with_sudo_if_required apt-get -y install libelf-dev
    check_return_code "\`apt-get install libelf-dev\` failed"

    #. Clone IOT Yocto ESE project
    IOT_YOCTO_ESE_DIR=$(mktemp -d)
    cd ${IOT_YOCTO_ESE_DIR}
    check_return_code "can't change working directory to ${IOT_YOCTO_ESE_DIR}"

    git clone https://github.com/intel/iotg-yocto-ese-main.git
    check_return_code "IOT Yocto ESE repository cloning failed"

    cd iotg-yocto-ese-main/
    git checkout 56aceb22632b9451c991529889f8c90def22153e

    #. Installing the libbpf library:
    LIBBPF_PATCHES_PATH=${IOT_YOCTO_ESE_DIR}/iotg-yocto-ese-main/backports/dunfell/recipes-connectivity/libbpf/libbpf

    LIBBPF_DIR=$(mktemp -d)
    cd ${LIBBPF_DIR}
    check_return_code "can't change working directory to ${LIBBPF_DIR}"

    git clone https://github.com/libbpf/libbpf.git
    check_return_code "libbpf repository cloning failed"

    cd libbpf/
    git checkout ab067ed3710550c6d1b127aac6437f96f8f99447

    git apply ${LIBBPF_PATCHES_PATH}/0001-libbpf-add-txtime-field-in-xdp_desc-struct.patch
    git apply ${LIBBPF_PATCHES_PATH}/0002-makefile-don-t-preserve-ownership-when-installing-fr.patch
    git apply ${LIBBPF_PATCHES_PATH}/0003-makefile-remove-check-for-reallocarray.patch
    cd src/
    make
    check_return_code "Libbpf make failed"

    run_with_sudo_if_required checkinstall -D --pkgname=libbpf -y
    check_return_code "Cannot install libbpf"

    run_with_sudo_if_required make install_uapi_headers
    check_return_code "Cannot install libbpf UAPI headers"

    run_with_sudo_if_required cp ${LIBBPF_DIR}/libbpf/include/uapi/linux/*.h /usr/include/linux/
    check_return_code  "Can not copy Libbpf headers"

    #### Open62541 installation
    OPEN62541_PATCHES_PATH=${IOT_YOCTO_ESE_DIR}/iotg-yocto-ese-main/recipes-connectivity/open62541/open62541-iotg/

    OPEN62541_DIR=$(mktemp -d)
    cd ${OPEN62541_DIR}
    check_return_code "can't change working directory to ${OPEN62541_DIR}"

    git clone https://github.com/open62541/open62541.git
    cd open62541/
    git checkout a77b20ff940115266200d31d30d3290d6f2d57bd

    git apply ${OPEN62541_PATCHES_PATH}/0001-CMakeLists.txt-Mark-as-IOTG-fork.patch
    git apply ${OPEN62541_PATCHES_PATH}/0001-fix-PubSub-Enable-dynamic-compilation-of-pubsub-exam.patch
    git apply ${OPEN62541_PATCHES_PATH}/0002-feature-PubSub-Use-libbpf-for-AF_XDP-receive-update-.patch
    git apply ${OPEN62541_PATCHES_PATH}/0003-feature-PubSub-add-support-for-AF_XDP-transmission.patch
    git apply ${OPEN62541_PATCHES_PATH}/0004-fix-PubSub-XDP-dynamic-compilation.patch
    git apply ${OPEN62541_PATCHES_PATH}/0005-fix-PubSub-update-example-to-set-XDP-queue-flags.patch
    git apply ${OPEN62541_PATCHES_PATH}/0006-test-PubSub-Configuration-used-for-compile-test.patch
    git apply ${OPEN62541_PATCHES_PATH}/0007-feature-PubSub-Add-ETF-LaunchTime-support-for-XDP-tr.patch
    git apply ${OPEN62541_PATCHES_PATH}/0008-fix-PubSub-AF_XDP-RX-release-mechanism-AF_PACKET-com.patch
    git apply ${OPEN62541_PATCHES_PATH}/0009-fix-PubSub-Fix-ETF-XDP-plugin-buffer-overflow.patch
    git apply ${OPEN62541_PATCHES_PATH}/0010-fix-PubSub-xdp-socket-cleanup-routine.patch
    git apply ${OPEN62541_PATCHES_PATH}/0011-fix-PubSub-fix-null-checking-possible-memleak-klocwo.patch
    git apply ${OPEN62541_PATCHES_PATH}/0012-fix-PubSub-remove-hardcoded-etf-layer-receive-timeou.patch

    mkdir build && cd build
    cmake -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_ENABLE_PUBSUB_ETH_UADP_ETF=ON -DUA_ENABLE_PUBSUB_ETH_UADP_XDP=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_PUBSUB_SOCKET_PRIORITY=ON -DUA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING=ON -DUA_ENABLE_PUBSUB_SOTXTIME=ON -DUA_ENABLE_SCHEDULED_SERVER=ON -DUA_BUILD_EXAMPLES=OFF -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/ ..
    check_return_code "Cannot build Open62541-iotg"

    run_with_sudo_if_required checkinstall -D  --pkgname=open62541-iotg -y
    check_return_code "Cannot install Open62541-iotg"
}

#. Set permissions for tools directory:
function set_permissions {
    chmod 0776 -R ${TCC_TOOLS_PATH}
    check_return_code "can't set permissions for ${TCC_TOOLS_PATH}"
}

if [ -z ${TCC_TOOLS_PATH} ]; then
    echo "TCC_TOOLS_PATH environment variable is not available. Make sure that you source env/vars.sh file"
    exit -1
fi

if [ "$EUID" -ne 0 ]; then
    echo "Script requires root privileges"
    echo "Run script with root privileges \`sudo -E ./install_host_dependencies.sh\` or Enter your password:"
    read -s USER_PASS
fi

apt_update
echo "SUCCESS: apt was updated"
install_itt
echo "SUCCESS: ittapi installed"
install_json
echo "SUCCESS: json-c installed"
install_host_tools_dependencies
echo "SUCCESS: host tools dependencies installed"
key_generation
echo "SUCCESS: key was generated"
xdp_support
echo "SUCCESS: XDP supporting libraries installed"
set_permissions
echo "SUCCESS: permissions for tools directory set"

echo "SUCCESS: all dependencies installed"
