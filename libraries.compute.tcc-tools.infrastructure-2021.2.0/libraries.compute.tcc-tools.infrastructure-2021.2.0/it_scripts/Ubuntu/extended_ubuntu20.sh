#!/bin/bash


#######################################################
#### TCC extended Ubuntu deployments script (2020) ####
#######################################################

BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="deploy_extended_script_$START_TIME.log"

declare -a WS_PACKAGE_LIST=("default-jre" "python3-pip" "graphviz" "xsltproc" "docbook-utils" \
    "dblatex" "xmlto" "xutils-dev" "jq" "cmake" "doxygen" "graphviz" "p7zip-full" "expect" \
    "gcovr" "csh" "dos2unix" "patchutils" "connect-proxy" "python-is-python3" "curl" "git-lfs" "autoconf" \
    "automake" "libtool" "libglib2.0-dev" "nfs-common" "gcc" "g++" "libjson-c-dev" "gettext" "bandit" \
    "clang-format-8" "clang-tidy-8")
declare -a CI_PACKAGE_LIST=("default-jre" "python3-pip" "graphviz" "xsltproc" "docbook-utils" \
    "dblatex" "xmlto" "xutils-dev" "jq" "cmake" "doxygen" "graphviz" "p7zip-full" "expect" \
    "gcovr" "csh" "dos2unix" "patchutils" "connect-proxy" "python-is-python3" "curl" "git-lfs" "autoconf" \
    "automake" "libtool" "libglib2.0-dev" "nfs-common" "gcc" "g++" "libjson-c-dev" "gettext" "bandit" \
    "clang-format-8" "clang-tidy-8")
declare -a YOCTO_PACKAGE_LIST=("gawk" "wget" "git-core" "diffstat" "unzip" "texinfo" \
    "gcc-multilib" "build-essential" "chrpath" "socat" "cpio" "python3" "python3-pip" \
    "python3-pexpect" "xz-utils" "debianutils" "iputils-ping" "python3-git" "python3-jinja2" \
    "libegl1-mesa" "libsdl1.2-dev" "pylint3" "xterm" "make" "xsltproc" "docbook-utils" "fop" \
    "dblatex" "xmlto")

warning_msg()
{
        echo -en "${BG_RED} $1 ${BG_DEF} \n"
}

notification_msg()
{
        echo -en "${BG_PUR} $1 ${BG_DEF} \n"
}

get_status()
{
        if [[ $? -eq 0 ]]; then
                notification_msg "$1 is complete successfull" >> $DEPLOY_LOG
        else
                warning_msg "$1 is failed" >> $DEPLOY_LOG
        fi
}

install_sw()
{
        for i in $@; do
                STEP_NAME="Installation of $i"
                notification_msg "$STEP_NAME"
                apt-get install -y "$i"
                get_status "$STEP_NAME"
        done
}

#############################
#### Check for SU rights ####
#############################
if [[ $EUID -ne 0 ]]; then
        warning_msg "This script must be run as root"
        exit 1
fi

#################################
#### Checking Ubuntu version ####
#################################
if [[ "$(lsb_release -r -s)" = "20.04" ]]; then
	notification_msg "Installation for Ubuntu 20.04 has started"
else
	warning_msg "Wrong Ubuntu version. This script is for Ubuntu 20.04 only"
	exit
fi

#######################
#### Set tmp proxy ####
#######################
export http_proxy=http://proxy-dmz.intel.com:911
export https_proxy=http://proxy-dmz.intel.com:912
export no_proxy=127.0.0.1,localhost,.intel.com

####################################################
#### Installation script selection for packages ####
####################################################
if [[ "$1" = "ws" ]]
        then
                notification_msg "You've chosen workstation script"
                PACKAGE_LIST=(${WS_PACKAGE_LIST[@]})
elif [[ "$1" = "ci" ]]
        then
                notification_msg "You've chosen CI script"
                PACKAGE_LIST=(${CI_PACKAGE_LIST[@]})
else
        notification_msg 'No parameters found. Please use "ci" for CI machine and "ws" for workstation'
        exit
fi


###############################
#### Extended SW installation ####
###############################
install_sw "${PACKAGE_LIST[@]}"

#############################################
#### Yocto related packages installation ####
#############################################
notification_msg "Installation of yocto related packages"
install_sw "${YOCTO_PACKAGE_LIST[@]}"

##############################
#### Git-lfs installation ####
##############################
notification_msg "Git-lfs installation"
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo -E bash
get_status "Adding of git-lfs repository"
apt-get update
apt-get install -y git-lfs
get_status "Git-lfs installation"

###############################
#### Coverage installation ####
###############################
pip3 install coverage
get_status "Coverage installation"


########################
#### Install docker ####
########################
chmod +x ./install_docker.sh
./install_docker.sh
get_status "Docker installation"
if [[ "$1" = "ci" ]]; then
	usermod -a -G  docker tccbot
fi

#####################################
#### Chameleonsocks installation ####
#####################################
git clone https://github.com/crops/chameleonsocks.git /opt/chameleonsocks
get_status "Git clone chameleonsocks"
https_proxy=https://proxy-dmz.intel.com:912 PROXY=proxy-dmz.intel.com PAC_URL=http://wpad.intel.com/wpad.dat /opt/chameleonsocks/chameleonsocks.sh --install
get_status "Chameleonsocks installation"

###########################
#### Install ittapi ####
###########################
notification_msg "ittapi installation"
git clone https://github.com/intel/ittapi.git
get_status "Ittapi repo download"
apt-get install -y cmake g++-multilib
get_status "ittapi dependencies installation"
cd ittapi
git checkout 66ebaee7a44fcdfb6ae052beb9507318781f3cd0
get_status "Switching to required commit"
python3 buildall.py
get_status "ittapi building"
mkdir -p /usr/include/ittnotify
cp build_linux/64/bin/libittnotify.a /usr/lib && \
cp -r include/* /usr/include/ittnotify && \
cp src/ittnotify/*.h /usr/include/ittnotify && \
cd ../
get_status "Copying itt parts to system folders"
rm -r /usr/include/ittnotify/fortran/win32 && \
rm -r /usr/include/ittnotify/fortran/posix/x86
get_status "Removing non-required itt parts"

###################################
#### Installation of open62541 ####
###################################
notification_msg "open62541 installation"
#. The dependencies are required for building XDP supporting libraries.
apt-get -y install python3-sphinx libelf-dev checkinstall
get_status "Installation of XDP pre-reqs"
CUR_DIR=$(pwd)
IOT_YOCTO_ESE_DIR=$(mktemp -d)
cd ${IOT_YOCTO_ESE_DIR}
get_status "changing working directory to ${IOT_YOCTO_ESE_DIR}"
git clone https://github.com/intel/iotg-yocto-ese-main.git
get_status "IOT Yocto ESE repository cloning"
#. Installing the libbpf library:
LIBBPF_PATCHES_PATH=${IOT_YOCTO_ESE_DIR}/iotg-yocto-ese-main/recipes-connectivity/libbpf/libbpf
LIBBPF_DIR=$(mktemp -d)
cd ${LIBBPF_DIR}
get_status "changing working directory to ${LIBBPF_DIR}"
git clone https://github.com/libbpf/libbpf.git
get_status "libbpf repository cloning"
cd libbpf/
git checkout ab067ed3710550c6d1b127aac6437f96f8f99447
git apply ${LIBBPF_PATCHES_PATH}/0001-libbpf-add-txtime-field-in-xdp_desc-struct.patch
git apply ${LIBBPF_PATCHES_PATH}/0002-makefile-don-t-preserve-ownership-when-installing-fr.patch
git apply ${LIBBPF_PATCHES_PATH}/0003-makefile-remove-check-for-reallocarray.patch
cd src/
make
get_status "Libbpf make running"
checkinstall -D  --pkgname=libbpf -y
get_status "Installation of libbpf"
make install_uapi_headers
get_status "Installation of libbpf UAPI headers"
cp ${LIBBPF_DIR}/libbpf/include/uapi/linux/*.h /usr/include/linux/
get_status "Copying Libbpf headers"
#### Open62541 installation
OPEN62541_PATCHES_PATH=${IOT_YOCTO_ESE_DIR}/iotg-yocto-ese-main/recipes-connectivity/open62541/open62541-iotg/
OPEN62541_DIR=$(mktemp -d)
cd ${OPEN62541_DIR}
get_status "Changing working directory to ${OPEN62541_DIR}"
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
cmake -DUA_ENABLE_PUBSUB=ON \
  -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
  -DUA_ENABLE_PUBSUB_ETH_UADP_ETF=ON \
  -DUA_ENABLE_PUBSUB_ETH_UADP_XDP=ON \
  -DUA_ENABLE_SUBSCRIPTIONS=ON \
  -DUA_ENABLE_PUBSUB_SOCKET_PRIORITY=ON \
  -DUA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING=ON \
  -DUA_ENABLE_PUBSUB_SOTXTIME=ON \
  -DUA_ENABLE_SCHEDULED_SERVER=ON \
  -DUA_BUILD_EXAMPLES=OFF \
  -DUA_ENABLE_AMALGAMATION=OFF \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_INSTALL_PREFIX=/usr/ ..
get_status "Building Open62541-iotg"
checkinstall -D  --pkgname=open62541-iotg -y
get_status "Installation of Open62541-iotg"
cd $CUR_DIR

#####################################
#### Python modules installation ####
#####################################
cat > python_prereqs.txt << EOF
##### PREREQUISITES for DSO and CC #####
##### siiptool #####
cryptography >= 2.5, < 3.0
click >= 7.0, < 8.0
##### common #####
paramiko >= 2.7, < 3.0
transitions >= 0.8.1, == 0.8.*
##### other ####
xmlrunner
unittest-xml-reporting
robotframework
robotframework-sshlibrary
numpy
matplotlib == 3.1.2
flake8
junit2html
bandit
EOF
pip3 install -r python_prereqs.txt
get_status "Python modules installation"

#############################
#### Export Java to path ####
#############################
if [[ "$1" = "ci" ]]; then
    echo 'export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64' >> /etc/environment
    get_status "Adding Java to env"
fi

###################################
#### Install EDK2 deps ####
###########################
apt install -y build-essential git uuid-dev iasl nasm
get_status "Install EDK2 deps"

####################
#### Git config ####
####################
if [[ "$1" = "ci" ]]; then
        git config --global user.name "tccbot"
	get_status "Git configuration for tccbot"
fi
git config --global --unset http.sslVersion
git config --global --add http.sslVersion tlsv1.2
get_status "Git configuration tls"


####################
#### Config MSS ####
####################
./config_mss.sh
get_status "MSS configuration"

##############################
#### Configure lib curses ####
##############################
export http_proxy="http://proxy-dmz.intel.com:911/"
export https_proxy="http://proxy-dmz.intel.com:911/"
export ftp_proxy="http://proxy-dmz.intel.com:911/"
export no_proxy=127.0.0.1,localhost,.intel.com
apt install libncurses5-dev libncursesw5-dev -y
get_status "Installation of libncurses"
perl -MCPAN -e 'install Curses'
get_status "Installation of perl Curses"


#############################################
#### Modify /etc/update-motd.d/00-header ####
#############################################
cat > /etc/update-motd.d/00-header << EEOOFF
#!/bin/sh
 
cat <<EOF
******************************************************************************
*  Welcome to the EUEC Computing Environment                                 *
*                                                                            *
*  Use of this system by UNAUTHORIZED persons or in an UNAUTHORIZED manner   *
*  is strictly prohibited.                                                   *
******************************************************************************
*                                                                            *
*  For support issues, please contact Engineering Service Desk(ESD)          *
*     ESD is available 24 hours, 7 days a week                               *
*     To contact EC Support for assistance, go to:                           *
*        http://it.intel.com                                                 *
*        OR call 1234 inside Intel                                           *
*                                                                            *
*                                                                            *
*     OR call +353 1 606 1234 outside Intel                                  *
*                                                                            *
*                                                                            *
******************************************************************************
EOF
 
uname -m -r -n
printf "%s\n" "$(lsb_release -s -d)"
EEOOFF


cat $DEPLOY_LOG
