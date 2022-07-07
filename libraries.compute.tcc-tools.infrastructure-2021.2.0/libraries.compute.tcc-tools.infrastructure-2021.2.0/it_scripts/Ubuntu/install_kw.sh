#!/bin/bash


########################################################
#### TCC Klocwork Ubuntu installation script (2019) ####
########################################################

BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="deploy_kw_script_$START_TIME.log"
KW_INSTALLER_NAME='kw-server-installer.20.1.0.97.linux64.sh'

declare -a PACKAGE_LIST=("libaio1" "libaio-dev" "lib32z1" "lib32ncurses5" "lsb-core" "libc6:i386" "libgcc1:i386" "lib32tinfo5" "libncursesw5:i386" "libnuma1")

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

#####################
#### Set KW user ####
#####################
notification_msg "Enter name of Klocwork user(e.g. tccbuild): "
while true; do
	read USERNAME
	id -u $USERNAME
        if [ "$?" == "1" ]; then
                warning_msg "No such user. Try again"
                continue
        else
                notification_msg "$USERNAME has been set as KW user"
                break
        fi
done

#######################################
#### Related packages installation ####
#######################################
dpkg --add-architecture i386
apt-get update
install_sw "${PACKAGE_LIST[@]}"

###############################
#### Download KW installer ####
###############################
sudo -u $USERNAME wget -P /opt https://af01p-igk.devtools.intel.com/artifactory/ScanTools-local/Klocwork/Server/KW20.1/Linux/$KW_INSTALLER_NAME \
	-e use_proxy=yes -e https_proxy=http://proxy-chain.intel.com:912
get_status "Download KW installer"

#######################
#### Run installer ####
#######################
chmod +x /opt/$KW_INSTALLER_NAME
if [ -d /opt/klocwork ]; then
	notification_msg "Klocwork already installed in folder /opt/klocwork"
	notification_msg "What do you want to do with current KW version"
	options=("Delete and re-install" "Keep and exit" "Keep and configure TC agent")
	select opt in "${options[@]}"
	do
		case $opt in
        	"Delete and re-install")
                	notification_msg "You've chosen: Delete and re-install"
			rm -rf /opt/klocwork
			get_status "Removing previous KW version"
			sudo -u $USERNAME mkdir /opt/klocwork
			sudo -u $USERNAME /opt/$KW_INSTALLER_NAME -a -i /opt/klocwork --license-server klocwork02p.elic.intel.com:7500
			get_status "Running KW installer"
			break
	                ;;
	        "Keep and exit")
			notification_msg "You've chosen: Keep and exit"
	                exit 1
	                ;;
	        "Keep and configure TC agent")
	                notification_msg "You've chosen: Keep and configure TC agent"
			break
	                ;;
			*) echo "invalid option $REPLY";;
	        esac
	done
else
	sudo -u $USERNAME mkdir /opt/klocwork
        sudo -u $USERNAME /opt/$KW_INSTALLER_NAME -a -i /opt/klocwork --license-server klocwork02p.elic.intel.com:7500
        get_status "Running KW installer"
	echo 'export PATH="$PATH:/opt/klocwork/bin"' >> /etc/bash.bashrc
        get_status "Adding to PATH"
        sed '/ip6-/ s?^?#?' -i /etc/hosts
        get_status "Fix hosts issue"
fi

############################
#### Configure TC agent ####
############################
TC_AGENTS_COUNT=$(ls -d  /opt/*/ | grep "buildAgent" | wc -l)
notification_msg "The script has found $TC_AGENTS_COUNT TeamCity Agents on this system"
while true; do
	if [ "$TC_AGENTS_COUNT" == "0" ]; then
		notification_msg "Do not forget to configure TC agent if going to use it!"
		break
	fi
	notification_msg "The list of available TC agent's paths:"
	ls -d  /opt/*/ | grep "buildAgent" 
	notification_msg "Please set the path for the required agent (e.g. /opt/buildAgent_test/):"
	read TC_AGENT_PATH

	if [ -f $TC_AGENT_PATH/conf/buildAgent.properties ]; then
		OWNER=$(stat -c '%U' $TC_AGENT_PATH/conf/buildAgent.properties)
		sudo -u $OWNER echo 'hasklocwork' >> $TC_AGENT_PATH/conf/buildAgent.properties
		get_status "TC Agent configuration"
		break
	elif [ "$TC_AGENT_PATH" == "skip" ]; then
		notification_msg "You've chosen to skip TC Agent configuration"
		break
	else
		notification_msg 'Path is not exist or does not contain TC config file. Try again or type "skip" to skip this step'
		continue
        fi
done


cat $DEPLOY_LOG
