# This file is run out of the srvd by install1.sh after it starts AFS.

umask 022

# Sun documents "uname -i" as the way to find stuff under /platform or
# /usr/platform.  We rely in the fact that the uname -i names are symlinks
# to the uname -m names, in order to save space on the root.  Be on the
# alert for changes.
platform=`uname -m`

CPUTYPE=`/sbin/machtype -c`; export HOSTTYPE
if [ "$CPUTYPE" = SPARC/4 ]; then
    echo "Setting monitor resolution..."
    /os/usr/platform/$platform/sbin/eeprom output-device=screen:r1152x900x94
    /os/usr/platform/$platform/sbin/eeprom fcode-debug?=true
fi

UPDATE_ROOT=/root; export UPDATE_ROOT
echo "Mounting hard disk's root partition..."
/etc/mount  $rootdrive $UPDATE_ROOT

cd /
echo "Making dirs on root"
mkdir /root/var
mkdir /root/usr
mkdir /root/proc
ln -s var/rtmp /root/tmp


echo "Mount var, usr , var/usr/vice..."
case $partitioning in
many)
    /sbin/mount  $vardrive /root/var
    /sbin/mount  $usrdrive /root/usr
    ;;
*)
    ;;
esac
mkdir /root/var/usr
mkdir /root/var/usr/vice
mkdir /root/var/tmp
mkdir /root/var/rtmp
chmod 1777 /root/var/tmp
chmod 1777 /root/var/rtmp

# If we have one partition of sufficient size, install all packages
# local and use the unmodified patches.  If we have many partitions or
# the one partition is smallish, install the packages from the non-local
# package list as symlinks and use the adjusted patches.
rootsize=`df -k /root | awk '{sz=$2;} END {print sz;}'`
if [ one = "$partitioning" -a 3145728 -le "$rootsize" ]; then
    echo "Installing packages and srvd local."
    cdnl=/cdrom
    patches=/patches
    subscr=sys_rvd.local
else
    cdnl=/cdrom/cdrom.link
    patches=/patches/patches.link
    subscr=sys_rvd
fi

case `uname -m` in
sun4u)
    suffix=u
    ;;
sun4m)
    suffix=m
    ;;
*)
    echo "unsupported architecture - contact Athena administration"
    exit 1
    ;;
esac

date >/tmp/start
echo "installing the os packages"
for i in `cat /cdrom/install-local-$suffix`; do
  echo $i
  cat /util/yes-file | pkgadd -R /root -d /cdrom $i
done 2>/dev/null
for i in `cat /cdrom/install-nolocal-$suffix`; do
  echo $i
  cat /util/yes-file | pkgadd -R /root -d $cdnl $i
done 2>/dev/null

echo "correction to pkg installation"
cp /cdrom/I* /root/var/sadm/system/admin/

echo "make it appear as  configured"
rm /root/etc/.UNC*
rm /root/etc/.sysidconfig.apps
cp /cdrom/.sysIDtool.state /root/etc/default/

echo "Installing Requested and Security patches for OS "
cat /util/yes-file | /util/patchadd -d -R /root -u -M \
    $patches `cat /patches/current-patches` 


echo "add/remove osfiles as needed\n"
sh /srvd/install/oschanges 2>/dev/null
date >/tmp/end
echo "the os part is installed"

echo "tracking the srvd"
/srvd/usr/athena/etc/track -d -F /srvd -T /root -W /srvd/usr/athena/lib \
	-s stats/$subscr slists/$subscr
echo "copying kernel modules from /srvd/kernel"
cp -p /srvd/kernel/fs/* /root/kernel/fs/

echo "Create devices and dev"
cd /root
/usr/sbin/drvconfig  -r devices -p /root/etc/path_to_inst
/usr/sbin/devlinks -t /root/etc/devlink.tab -r /root 
/usr/sbin/disks -r /root

chmod 755 /root/dev
chmod 755 /root/devices
cp /dev/null /root/reconfigure


cd /root
echo "Creating other files/directories on the pack's root..."
mkdir afs mit 
ln -s /var/usr/vice usr/vice
chmod 1777 /root/tmp 

echo "Finishing etc"
cp /dev/null etc/mnttab
cp /dev/null etc/.mnttab.lock
cp /dev/null etc/dumpdates
for i in `cat /srvd/usr/athena/lib/update/configfiles`; do
    cp -p /srvd$i /root$i
done

if=`ifconfig -au | awk -F: '/^[a-z]/ { if ($1 != "lo0") { print $1; exit; } }'`
if [ -z "$if" ]; then if=le0; fi
hostname=`echo $hostname | /usr/bin/tr "[A-Z]" "[a-z]"`
echo "Host name is $hostname"
echo "Gateway is $gateway"
echo "Address is $netaddr"
echo $hostname >etc/nodename
echo $hostname >etc/hostname.$if
echo $gateway >etc/defaultrouter
netmask=`cat /devices/cnbdrv:netmask`
echo "$netaddr  $netmask" > etc/inet/netmasks
echo "$netaddr	${hostname}.MIT.EDU $hostname" >>etc/inet/hosts

cd /root
if [ -r /srvd/etc/passwd ]; then
    cp -p /srvd/etc/passwd etc/passwd.local
    chmod 644 etc/passwd.local
    chown root etc/passwd.local
else
    cp -p /srvd/etc/passwd.fallback etc/passwd.local
fi
if [ -r /srvd/etc/shadow ]; then
    cp -p /srvd/etc/shadow etc/shadow.local
    chown root etc/shadow.local
else
    cp -p /srvd/etc/shadow.fallback etc/shadow.local
fi
cp -p etc/passwd.local etc/passwd
cp -p etc/shadow.local etc/shadow
chmod 600 etc/shadow
cp -p /srvd/etc/group etc/group
cp -p /srvd/etc/athena/athinfo.access etc/athena/athinfo.access
#ln -s ../var/adm/utmp etc/utmp
#ln -s ../var/adm/utmpx etc/utmpx
#ln -s ../var/adm/wtmp etc/wtmp
#ln -s ../var/adm/wtmpx etc/wtmpx
cp -p /srvd/etc/athena/*.conf etc/athena/
echo "Updating dm config"
cp -p /srvd/etc/athena/login/config etc/athena/login/config
echo "Editing rc.conf and version"
sed -e 	"s#^HOST=[^;]*#HOST=$hostname#
	s#^ADDR=[^;]*#ADDR=$netaddr#
	s#^NETDEV=[^;]*#NETDEV=$if#
	s#^MACHINE=[^;]*#MACHINE=$CPUTYPE#
	s#^SYSTEM=[^;]*#SYSTEM=Solaris#" \
	< /srvd/etc/athena/rc.conf > /root/etc/athena/rc.conf
rm -f /root/.rvdinfo
echo installed on `date` from `df -k / | tail -1 | awk '{print $1}'` \
	> /root/etc/athena/version
if [ $CUSTOM = Y ]; then
	if [ $PARTITION = Y ]; then
		echo custom install with custom partitioning \
			>> /root/etc/athena/version
	else
		echo custom install >> /root/etc/athena/version
	fi
fi
sed  -e "s/RVD/Workstation/g" < /srvd/.rvdinfo >> /root/etc/athena/version

echo "Updating vfstab"
rm -f etc/vfstab
case $partitioning in
many)
    sed "s/@DISK@/$drive/g" /srvd/etc/vfstab.std > etc/vfstab
    ;;
*)
    sed "s/@DISK@/$drive/g;/d0s5/d;/d0s6/d" /srvd/etc/vfstab.std > etc/vfstab
    ;;
esac
chmod 644 etc/vfstab
cp /dev/null etc/named.local


cd /var/usr/vice
for i in  CellServDB SuidCells 
	do
	cp -p /afs/athena/service/$i etc/
	chown root:root etc/$i
	chmod a+r etc/$i
done
echo "Copied afs service files into var/usr/vice/etc"

echo "turn fs slow"
/util/fastfs /root slow
/util/fastfs /root/usr slow
/util/fastfs /root/var slow

echo "Initializing var/adm and spool files "
cp /dev/null /root/var/adm/lastlog
cp /dev/null /root/var/adm/utmp
cp /dev/null /root/var/adm/utmpx
cp /dev/null /root/var/adm/wtmp
cp /dev/null /root/var/adm/wtmpx
cp /dev/null /root/var/adm/sulog
cp /dev/null /root/var/spool/mqueue/syslog
chmod 600 /root/var/adm/sulog /root/var/spool/mqueue/syslog
rm -f /root/var/spool/cron/crontabs/uucp
echo "Installing bootblocks on root "
installboot "/os/usr/platform/$platform/lib/fs/ufs/bootblk" "$rrootdrive"

echo "setting the boot device"
if [ $CUSTOM = Y ]; then
	/os/usr/platform/$platform/sbin/eeprom boot-device="$bootdevice"
else
	/os/usr/platform/$platform/sbin/eeprom boot-device="disk net"
fi

cd /root
cp /tmp/install.log /root/var/athena/install.log
case $REBOOT in
N)
    echo "after doing your customizations, type exit "
    echo "and the machine will reboot itself"
    /sbin/sh
    ;;
default)
    ;;
esac
echo "Unmounting filesystems and checking them"
cd /

umount /var/usr/vice > /dev/null 2>&1
fsck -y -F ufs $rcachedrive
case $partitioning in
many)
    umount /root/var > /dev/null 2>&1
    fsck -y -F ufs $rvardrive
    umount /root/usr
    fsck -y -F ufs $rusrdrive
    ;;
esac
/sbin/umount /root > /dev/null 2>&1
/usr/sbin/fsck -y -F ufs $rrootdrive
sleep 5

echo "Rebooting now"
reboot

