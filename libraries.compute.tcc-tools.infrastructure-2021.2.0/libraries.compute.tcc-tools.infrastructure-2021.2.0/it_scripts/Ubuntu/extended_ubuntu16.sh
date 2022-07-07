
#!/bin/bash


#######################################################
#### TCC extended Ubuntu deployments script (2019) ####
#######################################################

BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="deploy_extended_script_$START_TIME.log"

declare -a WS_PACKAGE_LIST=("default-jre" "python3-pip" "graphviz" "xsltproc" "docbook-utils" "dblatex" "xmlto" "xutils-dev" "jq" "cmake" "doxygen" "graphviz" "p7zip-full" "expect" "gcovr" "csh" "dos2unix" "patchutils" "connect-proxy")
declare -a CI_PACKAGE_LIST=("default-jre" "python3-pip" "graphviz" "xsltproc" "docbook-utils" "dblatex" "xmlto" "xutils-dev" "jq" "cmake" "doxygen" "graphviz" "p7zip-full" "expect" "gcovr" "csh" "dos2unix" "patchutils" "connect-proxy")
declare -a YOCTO_PACKAGE_LIST=("gawk" "wget" "git-core" "diffstat unzip" "texinfo" "gcc-multilib" "build-essential" "chrpath" "socat" "libsdl1.2-dev" "xterm")

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
if [[ "$(lsb_release -r -s)" = "16.04" ]]; then
	notification_msg "Installation for Ubuntu 16.04 has started"
else
	warning_msg "Wrong Ubuntu version. This script is for Ubuntu 16.04 only"
	exit
fi

#######################
#### Set tmp proxy ####
#######################
export http_proxy=http://proxy-chain.intel.com:911
export https_proxy=http://proxy-chain.intel.com:912

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

#############################
#### Python installation ####
#############################
notification_msg "Python installation"
add-apt-repository -y ppa:jonathonf/python-3.6
get_status "Adding of python 3.6 repository"
apt-get update
apt-get install -y python3.6 python3.6-dev
get_status "Python 3.6 installation"
wget https://bootstrap.pypa.io/get-pip.py -e use_proxy=yes -e https_proxy=http://proxy-chain.intel.com:912
get_status "Download get-pip"
python3.6 get-pip.py
get_status "Run get-pip"
pip3 install pyinstaller
get_status "Pyinstaller installation"
apt-get install -y libc6-dev
get_status "libc6-dev installation"
update-alternatives --install /usr/bin/python python /usr/bin/python2.7 1
update-alternatives --install /usr/bin/python python /usr/bin/python3.6 2

###############################
#### Coverage installation ####
###############################
pip3 install coverage
get_status "Coverage installation"

####################
#### GCC ugrade ####
####################
add-apt-repository ppa:ubuntu-toolchain-r/test
get_status "Adding GCC repo"
apt-get update
apt-get install -y gcc-7 g++-7
get_status "GCC installation"
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7 --slave /usr/bin/gcov gcov /usr/bin/gcov-7 --slave /usr/bin/gcov-tool gcov-tool /usr/bin/gcov-tool-7
get_status "GCC upgrade version"

########################
#### Install docker ####
########################
chmod +x ./install_docker.sh
./install_docker.sh
get_status "Docker installation"
if [[ "$1" = "ci" ]]; then
	usermod -a -G  docker tccbuild
fi

#####################################
#### Chameleonsocks installation ####
#####################################
git clone https://github.com/crops/chameleonsocks.git /opt/chameleonsocks
get_status "Git clone chameleonsocks"
https_proxy=https://proxy-chain.intel.com:912 PROXY=proxy-jf.intel.com PAC_URL=http://wpad.intel.com/wpad.dat /opt/chameleonsocks/chameleonsocks.sh --install
get_status "Chameleonsocks installation"

###########################
#### Install ittnotify ####
###########################
chmod +x ./install_ittnotify.sh
./install_ittnotify.sh
get_status "Ittnotify installation"

######################################
#### Installation of patched dpdk ####
######################################
cp /nfs/inn/disks/tcc_outgoing/Deploy/dpdk.tar.gz /tmp/
get_status "Copying of dpdk from share"
tar -xzf /tmp/dpdk.tar.gz -C /
get_status "Unpack of dpdk"

#################################
#### Installation of git-lfs ####
#################################
wget https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh
chmod +x script.deb.sh
./script.deb.sh
get_status "Adding of git-lfs repo"
apt install -y git-lfs
get_status "Installation of git-lfs"

#############################
#### Export Java to path ####
#############################
echo 'export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64' >> /etc/environment
get_status "Adding Java to env"

####################
#### Git config ####
####################
if [[ "$1" = "ci" ]]; then
        git config --global user.name "tccbuild"
	get_status "Git configuration for tccbuild"
fi
git config --global --unset http.sslVersion
git config --global --add http.sslVersion tlsv1.2
get_status "Git configuration tls"

##############################
#### Creation of tmp dirs ####
##############################
if [[ "$1" = "ci" ]]; then
        mkdir -p /opt/tmp/tgl_dl_dir
        mkdir -p /opt/tmp/tgl_sstates
        mkdir -p /opt/tmp/sstate
        mkdir -p /opt/tmp/dl_dir
        chmod -R a+wr /opt/tmp/
        chown -R tccbuild:intelall /opt
        mkdir -p /opt/team_city_cache/ehl/toolchain/
        chown -R tccbuild:intelall /opt/team_city_cache/ehl/toolchain
	ls -la /opt/tmp/tgl_dl_dir && \
	ls -la /opt/tmp/tgl_sstates && \
	ls -la /opt/tmp/sstate && \
	ls -la /opt/tmp/dl_dir && \
	ls -la /opt/team_city_cache/ehl/toolchain/
	get_status "Creation of tmp dirs"
fi

####################
#### Config MSS ####
####################
./config_mss.sh
get_status "MSS configuration"

##############################
#### Configure lib curses ####
##############################
export http_proxy="http://proxy-chain.intel.com:911/"
export https_proxy="http://proxy-chain.intel.com:911/"
export ftp_proxy="http://proxy-chain.intel.com:911/"
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
