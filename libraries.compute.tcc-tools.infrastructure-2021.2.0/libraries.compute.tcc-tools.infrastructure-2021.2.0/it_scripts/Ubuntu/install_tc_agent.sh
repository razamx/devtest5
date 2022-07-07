#!/bin/bash

###########################################################
#### TCC TeamCity agent linux deployment script (2019) ####
###########################################################

BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="deploy_tc_agent_script_$START_TIME.log"

SHARE_PATH=/nfs/inn/disks/tcc_outgoing/Deploy
TC_ARCHIVE=buildAgent2020.zip
TC_CONFIG=buildAgent.properties
SERVICE_NAME=teamcityagent.service
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
                status "$STEP_NAME"
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
#### Check if share is mounted ####
###################################
if [[ ! -d "$SHARE_PATH" ]]; then
	warning_msg "Share is not mounted. Path $SHARE_PATH is not available"
	exit 1
fi

###################################
#### Check if TC archive exists ####
###################################
if [[ ! -f "$SHARE_PATH/$TC_ARCHIVE" ]]; then
	warning_msg "$TC_ARCHIVE is not found in folder $SHARE_PATH"
        exit 1
fi

###################################
#### Check if TC config exists ####
###################################
if [[ ! -f "$SHARE_PATH/$TC_CONFIG" ]]; then
        warning_msg "$TC_CONFIG is not found in folder $SHARE_PATH"
        exit 1
fi

############################
#### Agent installation ####
############################
notification_msg "Please set agent's name: (e.g. nnltccsrv-01-quick1):"
read AGENT_NAME
unzip $SHARE_PATH/$TC_ARCHIVE -d /opt/buildAgent_$AGENT_NAME
get_status "Unzip of TC archive"
cp $SHARE_PATH/$TC_CONFIG /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
get_status "Copying of TC config"

#############################
#### Agent configuration ####
#############################
sed -i "s/^name=.*/name=$AGENT_NAME/g" /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
get_status "Setting agent's name($AGENT_NAME)"
notification_msg "Please enter Agent's PURPOSE: "
notification_msg 'NOTE: there should be only one BASE per system. If you need "-quick" use HOST'
options=("BASE" "HOST" "TARGET" "Skip")
select opt in "${options[@]}"
do
	case $opt in
        "BASE")
		notification_msg "You've chosen BASE"
		sed -i "s/^env.PURPOSE=.*/env.PURPOSE=$opt/g" /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
		get_status "TC PURPOSE configuration"
		break
		;;
        "HOST")
		notification_msg "You've chosen HOST"
		sed -i "s/^env.PURPOSE=.*/env.PURPOSE=$opt/g" /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
		get_status "TC PURPOSE configuration"
		break
		;;
        "TARGET")
		notification_msg "You've chosen TARGET"
		sed -i "s/^env.PURPOSE=.*/env.PURPOSE=$opt/g" /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
                get_status "TC PURPOSE configuration"
		notification_msg "Please set TARGET's platform (e.g. APL, TGL, EHL):"
                read TARGET_PLATFORM
		echo "env.PLATFORM=$TARGET_PLATFORM" >> /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
		get_status "TC PLATFORM configuration"
		notification_msg "Please set TARGET's hostname (e.g. nnltcc-001.inn.intel.com):"
		read TARGET_HOST
		echo "target_hostname=$TARGET_HOST" >> /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
		get_status "TC TARGET HOST configuration"
		notification_msg "Please set TARGET's root password:"
            	read TARGET_PASS
		echo "target_pass=$TARGET_PASS" >> /opt/buildAgent_$AGENT_NAME/conf/buildAgent.properties
		get_status "TC TARGET PASS configuration"
		break
		;;
        "Skip")
		break
		;;
        *) echo "invalid option $REPLY";;
	esac
done



###################################
#### Allow to execute agent.sh ####
###################################
chmod +x /opt/buildAgent_$AGENT_NAME/bin/agent.sh

#################################
#### Set rights for tccbuild ####
#################################
chown tccbuild:intelall -R /opt/buildAgent_$AGENT_NAME

###############################
#### Service configuration ####
###############################
if [[ ! -f "/etc/systemd/system/$SERVICE_NAME" ]]; then
	cp $SHARE_PATH/$SERVICE_NAME /etc/systemd/system/
	sed -i "s/buildAgentX/buildAgent_$AGENT_NAME/g" /etc/systemd/system/$SERVICE_NAME
	get_status "Service sript configuration"
	chmod 644 /etc/systemd/system/$SERVICE_NAME
        systemctl enable $SERVICE_NAME
	systemctl daemon-reload
	get_status "Services update"
else
        awk -i inplace -v "var=ExecStart=/opt/buildAgent_$AGENT_NAME/bin/agent.sh start" '/^ExecStart=.*/ && !x {print var; x=1} 1' /etc/systemd/system/$SERVICE_NAME
	awk -i inplace -v "var=ExecStop=-/opt/buildAgent_$AGENT_NAME/bin/agent.sh stop" '/^ExecStop=.*/ && !x {print var; x=1} 1' /etc/systemd/system/$SERVICE_NAME
	get_status "Service sript configuration"
	chmod 644 /etc/systemd/system/$SERVICE_NAME
	systemctl daemon-reload
        get_status "Services update"
fi

cat $DEPLOY_LOG
notification_msg "For starting build agent change account to "tccbuild" and run "/opt/buildAgent_$AGENT_NAME/bin/agent.sh start""
