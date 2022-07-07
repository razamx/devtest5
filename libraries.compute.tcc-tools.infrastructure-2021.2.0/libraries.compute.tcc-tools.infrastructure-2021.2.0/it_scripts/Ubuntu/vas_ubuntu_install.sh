#!/bin/bash

BGRED='\033[41m'
NORMAL='\033[0m'

#ubuntu version
#ub_ver=`echo $(lsb_release -r -s)`

#Red notification
red_notif()
{
	echo -en "${BGRED} $1 ${NORMAL} \n" 
}
red_notif "Checking for updates"
#apt-get update
#apt-get -y dist-upgrade

red_notif "Start VAS client installation"
export VASUSER="russia_lde_joiner"
export VASPASSWD="IypHXiosYU1GaCI2RDw8KztmfUsjTzI5"
export VASOU="OU=labHosts,OU=inn,OU=UNIX,OU=Engineering Computing,OU=Resources,DC=ccr,DC=corp,DC=intel,DC=com"
export CORPDOMAIN="ccr"
export LDE_DOWNLOAD_PATH="http://lde.ims.intel.com/repository"
export HOSTNAME=`hostname | awk -F"." '{ print $1; }'` 
export DNSDOMAIN="inn.intel.com"
export ARCH="ia32e"

# download packages
#wget -q -nd -nH --no-parent -r -A 'vasclnts*.deb' -P /etc/intel/packages ${LDE_DOWNLOAD_PATH}/lde_3dparty/CPIM/${ARCH}/
wget -P /etc/intel/packages http://ec-xds.intel.com/vas/4.1/site/QAS_4_1_3_23101-Site/client/linux-x86_64/vasclnts_4.1.3-23101_amd64.deb  -e use_proxy=yes -e http_proxy=http://proxy-chain.intel.com:911
# install packages
dpkg -i /etc/intel/packages/vasclnts*.deb
  
#sync time
/opt/quest/bin/vastool timesync -d $CORPDOMAIN.corp.intel.com

# download lde-vastool
wget -q -O /opt/quest/bin/lde-vastool ${LDE_DOWNLOAD_PATH}/lde_3dparty/CPIM/lde-vastool/lde-vastool-${ARCH} -e use_proxy=yes -e http_proxy=http://proxy-chain.intel.com:911
chmod +x /opt/quest/bin/lde-vastool
export VASOU2=$(echo ${VASOU} | cut -d "," -f2-)

#Checking vasd.sysv
FILE_VASD="/etc/init.d/vasd.sysv"
if [ -e "$FILE_VASD" ]
    then
		unlink /etc/init.d/vasd
	    cp /etc/init.d/vasd.sysv /etc/init.d/vasd
		
fi

systemctl daemon-reload

# join to domain
/opt/quest/bin/lde-vastool -u ${VASUSER} -w "${VASPASSWD}" join -f --skip-config -l -c "${VASOU}" -p "${VASOU2}" -n "${HOSTNAME}v.$DNSDOMAIN" -w $CORPDOMAIN.corp.intel.com
  
# configure PAM
mkdir -p /lib/security
/opt/quest/bin/vastool configure nss
/opt/quest/bin/vastool configure pam
ln -sf /opt/quest/lib64/security/pam_vas3.so /lib/security/pam_vas3.so
ls -al /lib/security/pam_vas3.so
#ln -s /opt/quest/lib64/nss/libnss_vas4.so.2 /lib/x86_64-linux-gnu/libnss_vas4.so.2
#ls -al /lib/x86_64-linux-gnu/libnss_vas4.so.2


#service vasd start
#service vasd status
if [ -e "$FILE_VASD" ]
    then
        update-rc.d vasd defaults
fi

update-rc.d vasd enable
service vasd start
service vasd status
systemctl enable vasd

red_notif "Checking status of installed VAS client"
/opt/quest/bin/vastool status

#Update rpcbind fix.
if grep "autofs" /etc/network/if-up.d/rpcbind ; then
	cat << EOF > /etc/network/if-up.d/rpcbind
#!/bin/sh
service rpcbind restart && service ypbind restart && sleep 15s && /etc/init.d/autofs restart && service vasd restart
  
EOF
	chmod +x /etc/network/if-up.d/rpcbind
	cat /etc/network/if-up.d/rpcbind
	invoke-rc.d autofs restart
	/etc/network/if-up.d/rpcbind
fi
red_notif "The end of VAS installation"