#!/bin/bash


#######################################################
#### TCC extended Ubuntu deployments script (2019) ####
#######################################################

BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="global_sudo_config_script_$START_TIME.log"

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

###################################
#### Global sudo configuration ####
###################################
ln -s /nfs/site/itools/em64t_linux26 /usr/intel
get_status "itools simlink creation"
ln -s /nfs/site/gen/adm/sudo /etc/sudo
get_status "sudo simlink creation"
cat << EOF > /etc/profile.d/sudo.sh
alias sudo='/usr/intel/bin/sudo'
EOF
cat << EOF > /etc/profile.d/sudo.csh
alias sudo '/usr/intel/bin/sudo'
EOF
chmod a+x /etc/profile.d/sudo.*
echo "alias sudo='/usr/intel/bin/sudo'" >> /etc/bash.bashrc
get_status "Configuration of sudo"
echo "export PATH=$PATH:/usr/intel/bin" >> /etc/bash.bashrc
get_status "Configuration of PATH"
warning_msg "Do not forget to reboot system" >> $DEPLOY_LOG

cat $DEPLOY_LOG

