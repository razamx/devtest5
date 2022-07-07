#!/bin/bash

#######################################################
#### TCC Ubuntu MSS complience script (2019) ####
#######################################################

BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="deploy_mss_script_$START_TIME.log"


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
elif [[ "$(lsb_release -r -s)" = "20.04" ]]; then
        notification_msg "Installation for Ubuntu 20.04 has started" 
else
        warning_msg "Wrong Ubuntu version. This script is for Ubuntu 16.04 and 20.04 only"
        exit
fi


#########################
#### Install package ####
#########################
apt install -y unattended-upgrades
get_status "Installation of unattended-upgrades"

#######################
#### Check package ####
#######################
dpkg-query -l unattended-upgrades

#################################
#### create cron reboot task ####
#################################
(crontab -l; echo "0 4 * * 0 /sbin/shutdown -r +5 >> /var/log/reboot.log 2>&1") | crontab -
get_status "Cron configuration"

#######################################
#### configure unattended-upgrades ####
#######################################
sed -i 's/^.*\(".*ESM.*$\)/\1/' /etc/apt/apt.conf.d/50unattended-upgrades
get_status "unattended-upgrades ESM configuration"
sed -i 's/^.*\(".*-security.*$\)/\1/' /etc/apt/apt.conf.d/50unattended-upgrades
get_status "unattended-upgrades security configuration" 
sed -i 's/^.*\("${distro_id}:${distro_codename}".*$\)/\1/' /etc/apt/apt.conf.d/50unattended-upgrades
get_status "unattended-upgrades main configuration"

############################
#### allow auto updates ####
############################
cat > /etc/apt/apt.conf.d/20auto-upgrades << EOF
APT::Periodic::Update-Package-Lists "1";
APT::Periodic::Download-Upgradeable-Packages "1";
APT::Periodic::AutocleanInterval "7";
APT::Periodic::Unattended-Upgrade "1";
EOF
get_status "Auto upgrades configuration"

####################################
#### Install Intel certificates ####
####################################
wget -P /tmp  http://certificates.intel.com/repository/certificates/IntelSHA2RootChain-Base64.zip
get_status "Downloading of SHA certificates"
wget -P /tmp  http://certificates.intel.com/repository/certificates/Intel%20Root%20Certificate%20Chain%20Base64.zip
get_status "Downloading of Root certificates"
mkdir /usr/local/share/ca-certificates/intel
chmod 755 /usr/local/share/ca-certificates/intel
unzip /tmp/IntelSHA2RootChain-Base64.zip -d /usr/local/share/ca-certificates/intel
get_status "Extracting of SHA certificates"
unzip /tmp/Intel\ Root\ Certificate\ Chain\ Base64.zip -d /usr/local/share/ca-certificates/intel
get_status "Extracting of Root certificates"
update-ca-certificates --verbose --fresh
get_status "Adding of Intel certificates"

###################################
#### Linux credential scanning ####
###################################
git clone https://gitlab.devtools.intel.com/kmartin1/nexpose-iags-public-key-scripts.git  linux_cred_scan
chmod +x ./linux_cred_scan/GER/add_iags_ger_pubkey.sh
./linux_cred_scan/GER/add_iags_ger_pubkey.sh
get_status "Adding of linux credential scanning"

cat $DEPLOY_LOG
