#!/bin/bash

###############################################
#### TCC docker installation script (2019) ####
###############################################


BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
DEPLOY_LOG='deploy_docker_script.log'

declare -a REL_PACKAGE_LIST=("apt-transport-https" "ca-certificates" "curl" "software-properties-common")

warning_msg()
{
        echo -en "${BG_RED} $1 ${BG_DEF} \n"
}

notification_msg()
{
        echo -en "${BG_PUR} $1 ${BG_DEF} \n"
}

status()
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
#### Dependencies installation ####
###################################
apt-get update
install_sw "${REL_PACKAGE_LIST[@]}"

######################################
#### Add Dockers official GPG key ####
######################################
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
status "Adding docker GPG key"

###############################
#### Setting up repository ####
###############################
add-apt-repository \
	"deb [arch=amd64] https://download.docker.com/linux/ubuntu \
	$(lsb_release -cs) \
	stable"
status "Adding docker repo"
apt-get update
apt-get install -y docker-ce
status "Docker package installation"

#########################
#### Configuring DNS ####
#########################
#THIS SETTING WILL BE EOL SOON
#cat > /etc/docker/daemon.json << EOF
#{
#  "dns":  ["10.125.233.5", "10.125.233.6", "10.248.2.1"]
#}
#EOF
#status "DNS configuration"

#############################
#### Proxy configuration ####
#############################
HTTPS_PROXY="https_proxy=http://proxy-dmz.intel.com:912"
HTTP_PROXY="http_proxy=http://proxy-dmz.intel.com:911"
NO_PROXY="no_proxy=127.0.0.1,localhost,.intel.com"
mkdir -p /etc/systemd/system/docker.service.d
echo -e '[Service]\nEnvironment="'$HTTPS_PROXY'" "'$NO_PROXY'"' >> /etc/systemd/system/docker.service.d/https-proxy.conf
echo -e '[Service]\nEnvironment="'$HTTP_PROXY'" "'$NO_PROXY'"' >> /etc/systemd/system/docker.service.d/http-proxy.conf
status "Proxy configuration"

###########################
#### Restarting docker ####
###########################
sudo systemctl daemon-reload
sudo systemctl restart docker
service docker restart
status "Docker restart"

########################
#### Testing docker ####
########################
docker run hello-world
status "Docker testing"

cat $DEPLOY_LOG

