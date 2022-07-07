#!/bin/bash

####################################################
#### TCC basic Ubuntu deployments script (2019) ####
####################################################


BG_RED='\033[41m'
BG_PUR='\033[44m'
BG_DEF='\033[0m'
START_TIME=$(date +'%F_%T')
DEPLOY_LOG="deploy_basic_script_$START_TIME.log"

declare -a WS_PACKAGE_LIST=("nis" "autofs" "mc" "ssh" "ntp" "ntpdate" "apt-file" "cifs-utils" "nfs-common" "sshpass" "socat" "git" "smbclient")
declare -a CI_PACKAGE_LIST=("nis" "autofs" "mc" "ssh" "ntp" "ntpdate" "apt-file" "cifs-utils" "nfs-common" "sshpass" "socat" "git" "smbclient")

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
	for i in "${PACKAGE_LIST[@]}"; do
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

##############################
#### Change root password ####
##############################
#echo -en "\033[37;1;41m Type password for ROOT \033[0m \n"
notification_msg "Type password for ROOT:"
while true; do
        passwd
        if [ $? != 0 ]; then
		warning_msg "Password change has been failed!"
                continue
        else
                notification_msg "Password was chandged, well done!"
                break
        fi
done


#Add proxy to /etc/apt/apt.conf.d/00aptitude file
STEP_NAME="Adding proxy to apt config"
echo 'Acquire::http::Proxy "http://proxy-chain.intel.com:911/";' >> /etc/apt/apt.conf.d/00aptitude
get_status "$STEP_NAME"

######################
####Update-upgrade####
######################
apt-get remove -y libappstream3
STEP_NAME="update + dist-upgrade"
apt-get update && apt-get -y dist-upgrade
get_status "$STEP_NAME"



###############################
#### Basic SW installation ####
###############################
install_sw


#############################################################
#### Configure sshd to allow connections to host via ssh ####
#############################################################

sed -i 's/^.*PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
get_status "Allow ssh connection by root"
service ssh restart
get_status  "ssh restart"

###################################################################
#### Add passwordless authorization to host from nnvmon001(IT) ####
###################################################################
notification_msg "Configuring sshd"
mkdir -p /root/.ssh
get_status "make dir /root/.ssh"
echo "ssh-dss AAAAB3NzaC1kc3MAAAEBALM/c78MC5BBFxh3o/NAu84wFVoIjjQDj3f7cHfMvX6r+g1yXhlXXzSyyq4WxsxkJPkUELkRYjQPO9UceVKp/cGWyHVDtHDIY2JH1i4OCF0vsDXBQjSmmT2zFh53sQ+mDfGz03CLp6bu/VkdqNM67FsIYPnB/ZtLe50YlUpM85DNvSAYJtMvE4o05kskwba/X5C1Aa2Xe04ud/bS2/LxHNP+9UBsBamYOmC7pRWZ+8Y5LOnHOcXqCLz4P9BfvBltJegGyKunEyf0YP+N5WcxhYjBAmS16d5cLZv+r7IXL11Z+bnB4dv2JkmC5lr+4OOPAvP7qxJmjziTOa2PCuU9rjUAAAAVAISeAtyyRSgjZG7a12lwMZWSO3FjAAABAGLH9pEzs7/o7bMEZIXDkPUfiOzeMSqfQWZriBtF87dwMfSG6B9mBgxIih0IKwE4HDyWq33rwHqRIWBt+UWklfTqZk7yEoonqt4bVIZ6nFbpQ4yxGBOoeDV5qpAoGxJqVrjByb2uTsIME/1I+ErDCNbMuoAy4RLpEC/jYl6wYxfgae3npVUo9MbWcim/QMuf1+LC/MCRMvkmvVB5hpgMKX3NdCcvD4qtAjfk4MjEaa5i/H7gFsFkgrU1mJ5QuwOxKD8NcnntrB4iq1l68K9zAS+/7VqQQSjdUZWsqKXuXMkfMbtzpnIJoMSMeFFu0PdvTlSVMJIIPPdDDyoda/Wej10AAAEBAIo3CID9O8xxQ9SbMiSn8DvdQmXabZJkd5nK6z5PKFy7sgNJuFgo8i/B6JaTumQOGexOmIsctaWyFmL7nvNpV7sFqILbxULw0HBTufs+tv+gP/4znS2rhSwmQqsGbQa7duZDJwk4Xr2kjX/Ldl/ANRMOeL9V6D0B0NuRJMeYdc4pMVOf8CJZpNYNCVNEmPwFqEa9/Z2HdiK7VTGCqjJ7aPD8OsEhe7gsnn3CprjmheBN5au6LQs8fnqK/ti45VfiF9DECCmmM90bKVKN1jITOtHRQAqdEqnPueKerJ5JiiU1v9/GftFXARVWWKuKHsdNhTZD8CRlz12wmsMDGyRsNsU=" >> /root/.ssh/authorized_keys 
get_status "adding IT ssh key to authorized"
#Please do not execute strings below on RHEL* systems, this is for Ubuntu/SLES only
echo "PubkeyAcceptedKeyTypes=+ssh-dss" >> /etc/ssh/sshd_config
get_status "Configuring PubkeyAcceptedKeyTypes"
/etc/init.d/ssh restart
get_status "ssh restart"

##########################################
#### Configure domain and NIS servers ####
##########################################
cat  > /etc/yp.conf << EOF
ypserver nis-host
ypserver nis-host1
ypserver nis-host2
EOF
get_status "Configuration of yp"

######################################
#### Configure /etc/nsswitch.conf ####
######################################
cat > /etc/nsswitch.conf << EOF
passwd:         files nis
group:          files nis
shadow:         files

hosts:          files dns
networks:       files nis

protocols:      files
services:       files nis
ethers:         files
rpc:            files nis

netgroup:       files nis
automount:      files nis

EOF
get_status "Configuration of nsswitch"

####################################################
#### Apply fix that rpcbind starts after reboot ####
####################################################
cat << EOF > /etc/network/if-up.d/rpcbind
#!/bin/sh
service rpcbind start && service ypbind restart && sleep 15s && /etc/init.d/autofs restart
EOF
get_status "Configuration of rpcbind"
chmod +x /etc/network/if-up.d/rpcbind


####################
#### Fix autofs ####
####################
echo 'TIMEOUT=14400' >> /etc/default/autofs
echo 'BROWSE_MODE="yes"' >> /etc/default/autofs
printf "\n" >> /etc/default/autofs
get_status "Fix autofs"

#################################
#### Update /etc/auto.master ####
#################################
echo '+auto.master.linux' > /etc/auto.master
get_status "Update auto.master"

########################
#### Restart autofs ####
########################
invoke-rc.d autofs restart
get_status "autofs restart"
/etc/network/if-up.d/rpcbind
get_status "test rpcbind"

####################
#### Check shares ####
####################
ls -al /nfs/inn/proj
get_status "Test NFS"

################################
#### Configure Common Tools ####
################################
# do not uncomment 
#ln -s /nfs/site/itools/em64t_linux26 /usr/intel

########################
#### Configure sudo ####
########################
#ln -s /nfs/site/gen/adm/sudo /etc/sudo

	
########################################
#### Edit ntpdate for synk datetime ####
########################################
sed -i 's/^NTPDATE_USE_NTP_CONF=no/NTPDATE_USE_NTP_CONF=yes/' /etc/default/ntpdate
get_status "Fix ntpdate"

##############################
#### Create /etc/ntp.conf ####
##############################
cat > /etc/ntp.conf << EOF
driftfile /var/lib/ntp/ntp.drift
# Enable this if you want statistics to be logged.
statsdir /var/log/ntpstats/

statistics loopstats peerstats clockstats
filegen loopstats file loopstats type day enable
filegen peerstats file peerstats type day enable
filegen clockstats file clockstats type day enable

#server ntp.ubuntu.com
restrict ntp-host1 nomodify
server ntp-host1 maxpoll 12
#
restrict ntp-host2 nomodify
server ntp-host2 maxpoll 12
#
restrict ntp-host3 nomodify
server ntp-host3 maxpoll 12

restrict default nomodify noserve nopeer notrust
restrict 127.0.0.1

disable auth

EOF
get_status "Configuration of ntp"

###################################################
#### Allows login into GUI, using any username ####
###################################################
echo 'greeter-show-manual-login=true' >> /usr/share/lightdm/lightdm.conf.d/50-greeter-wrapper.conf
get_status "Fix gui login"

###############################
#### Create alias for sudo ####
###############################
#cat << EOF > /etc/profile.d/sudo.sh
#alias sudo='/usr/intel/bin/sudo'
#EOF
#cat << EOF > /etc/profile.d/sudo.csh
#alias sudo '/usr/intel/bin/sudo'
#EOF
#chmod a+x /etc/profile.d/sudo.*

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

###############
#### Proxy ####
###############
#tee /etc/environment > /dev/null <<'ENDENVIRONMENT'
#export HTTP_PROXY=http://proxy-chain.intel.com:911
#export http_proxy=http://proxy-chain.intel.com:911
#export HTTPS_PROXY=http://proxy-chain.intel.com:912
#export https_proxy=http://proxy-chain.intel.com:912
#export no_proxy=127.0.0.1,localhost,.intel.com
#export FTP_PROXY=http://proxy-chain.intel.com:912
#export ftp_proxy=http://proxy-chain.intel.com:912
#export ALL_PROXY=http://proxy-chain.intel.com:912
#export all_proxy=http://proxy-chain.intel.com:912
#export ANT_ARGS="-autoproxy"
#export ANT_OPTS="-Dhttp.proxyHost=proxy-chain.intel.com -Dhttp.proxyPort=911"
#PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games"
#ENDENVIRONMENT

#########################
#### Instalation kerberos ####
#########################
wget https://github.intel.com/raw/mwchrist/multirealm_pam_krb5/master/krb_install_linux_time_repos.sh
chmod +x krb_install_linux_time_repos.sh
./krb_install_linux_time_repos.sh
get_status "Kerberos configuration"

###########################
#### Install libcurces ####
###########################
#export http_proxy="http://proxy-chain.intel.com:911/"
#export https_proxy="http://proxy-chain.intel.com:911/"
#export ftp_proxy="http://proxy-chain.intel.com:911/"
#apt install libncurses5-dev libncursesw5-dev -y
#perl -MCPAN -e 'install Curses'
#get_status "libcurces fix"

cat $DEPLOY_LOG
