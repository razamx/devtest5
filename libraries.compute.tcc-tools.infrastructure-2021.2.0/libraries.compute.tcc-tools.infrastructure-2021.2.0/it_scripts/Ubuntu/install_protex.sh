#!/bin/bash


########################################################
#### TCC Protex Ubuntu installation script (2019) ####
########################################################

BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="deploy_protex_script_$START_TIME.log"
PROTEX_INSTALLER_NAME='blackduck-client-linux-amd64.bin'
PROTEX_ARCHIVE_NAME='blackduck-client-linux-amd64.zip'
PROTEX_NAME='blackduck-client-linux-amd64'
PROTEX_PATH='/opt/blackduck/protexIP'
SHARE_PATH='/nfs/inn/disks/tcc_outgoing/Deploy'

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

###################################
#### Check if TC archive exists ####
###################################
if [[ ! -f "$SHARE_PATH/$PROTEX_ARCHIVE_NAME" ]]; then
        warning_msg "$PROTEX_ARCHIVE_NAME is not found in folder $SHARE_PATH"
        exit 1
fi

#########################
#### Set Protex user ####
#########################
notification_msg "Enter name of Klocwork user(e.g. tccbuild): "
while true; do
        read USERNAME
        id -u $USERNAME
        if [ "$?" == "1" ]; then
                warning_msg "No such user. Try again"
                continue
        else
                notification_msg "$USERNAME has been set as Protex user"
                break
        fi
done

#######################
#### Run installer ####
#######################
if [ -d "$PROTEX_PATH" ]; then
        notification_msg "Protex already installed in folder /opt/protex"
        notification_msg "What do you want to do with current Protex version"
        options=("Delete and re-install" "Keep and exit" "Keep and configure TC agent")
        select opt in "${options[@]}"
        do
                case $opt in
                "Delete and re-install")
                        notification_msg "You've chosen: Delete and re-install"
                        rm -rf $PROTEX_PATH
                        get_status "Removing previous Protex version"
                        sudo -u $USERNAME unzip $SHARE_PATH/$PROTEX_ARCHIVE_NAME -d /opt
			get_status "Unzip of Protex archive"
			sudo -u $USERNAME chmod +x /opt/$PROTEX_NAME/$PROTEX_INSTALLER_NAME
			sudo -u $USERNAME /opt/$PROTEX_NAME/$PROTEX_INSTALLER_NAME -i silent
			get_status "Running Protex installer"
                        rm -rf /opt/$PROTEX_NAME
			get_status "Removing Protex installer"
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
        sudo -u $USERNAME unzip $SHARE_PATH/$PROTEX_ARCHIVE_NAME -d /opt
        get_status "Unzip of Protex archive"
	sudo -u $USERNAME chmod +x /opt/$PROTEX_NAME/$PROTEX_INSTALLER_NAME
        sudo -u $USERNAME /opt/$PROTEX_NAME/$PROTEX_INSTALLER_NAME -i silent
        get_status "Running Protex installer"
	echo "export PATH=\"\$PATH:$PROTEX_PATH/bin\"" >> /etc/bash.bashrc
	get_status "Adding to PATH"
	rm -rf /opt/$PROTEX_NAME
	get_status "Removing Protex installer"
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
                sudo -u $OWNER echo 'hasprotex' >> $TC_AGENT_PATH/conf/buildAgent.properties
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
notification_msg "Protex installation requirs reboot. Do not forget to make it manually"
