#!/bin/sh -euk

### This is the second script file in the Athena workstation
### installation program.  It is called by the first script,
### athenainstall.

### $Header: /afs/dev.mit.edu/source/repository/packs/install/platform/sun4/phase2.sh,v 1.23 1997-06-20 19:05:37 miki Exp $
### $Locker:  $

echo "Set some variables"
PATH=/sbin:/os/usr/bin:/srvd/bin/athena:/os/etc:/os/usr/sbin:/bin:/etc:/sbin:/usr/sbin
export PATH
umask 2

echo "Custom installation ? (will default to n after 60 seconds)[n]"

case x`/util/to 60` in
xtimeout|xn|xN|x)
     CUSTOM=N; echo "Doing standard installation"; export CUSTOM;;
*)
     CUSTOM=Y; echo "Doing custom installation"; export CUSTOM;;
esac

# Define the partitions
echo "Define the partitions"

drive=c0t3d0

rootdrive=/dev/dsk/${drive}s0
rrootdrive=/dev/rdsk/${drive}s0

cachedrive=/dev/dsk/${drive}s3
rcachedrive=/dev/rdsk/${drive}s3

usrdrive=/dev/dsk/${drive}s5
rusrdrive=/dev/rdsk/${drive}s5

vardrive=/dev/dsk/${drive}s6
rvardrive=/dev/rdsk/${drive}s6

export drive rootdrive rrootdrive cachedrive rcachedrive usrdrive rusrdrive
export vardrive rvardrive

echo "Installing on ${drive}."

case $CUSTOM in
N)
  echo "standard installation - 8.1"
  ln -s /afs/athena.mit.edu/system/sun4x_55/srvd-8.1 /tmp/srvd
  ln -s /afs/athena.mit.edu/system/sun4x_55/os /tmp/os
  ;;

Y)
   echo "custom installation"
   echo "Which rev do you want to install ? "
   read buff
   case "$buff" in
   7.7)
       echo "installing 7.7"
       ln -s /afs/athena.mit.edu/system/sun4m_53/srvd.77 /tmp/srvd
       ;;
   8.0)
       echo "installing 8.0"
       ln -s /afs/athena.mit.edu/system/sun4m_54/srvd.80 /tmp/srvd
       ln -s /afs/athena.mit.edu/system/sun4m_54/os /tmp/os 
       ;;
   *)
       echo "installing 8.1"
       ln -s /afs/athena.mit.edu/system/sun4x_55/srvd-8.1 /tmp/srvd
       ln -s /afs/athena.mit.edu/system/sun4x_55/os /tmp/os
       ;;
   esac
   echo "done choosing rev"
   ;;
esac

ls -l /srvd
ls -l /tmp/srvd
ls -l /os
ls -l /tmp/os

echo "formatting  "
DISK=`/sbin/machtype -r`
export DISK
echo $DISK

case $CUSTOM in
Y)
   echo "Partitioning the disk yourself? (will default after 60 seconds) [n]"
   case x`/util/to 60` in
   xtimeout|xn|xN|x)
     PARTITION=N; echo "Doing standard partitions"; export PARTITION
     ;;
   *)
     PARTITION=Y; echo "Doing custom partitions"; export PARTITION
     ;;
   esac
   ;;
*)     

     PARTITION=N; echo "Doing standard partitions"
     ;;
esac

case $PARTITION in
Y)
     echo "The rest of the installation assumes that
     partition 0 is / and needs about 30MB;
     partition 1 is swap;
     partition 3 is AFS cache;
     partition 5 is /usr and needs 50MB;
     partition 6 is /var and needs at least 120MB "
     sleep 10
     /usr/sbin/format
     ;;
*)
     case $DISK in
     SUN0424)
        echo "formatting SUN0424"
        cat /util/format.input.SUN0424 | \
		/usr/sbin/format ${drive} >/dev/null 2>&1
        ;;
     SUN0535)
        echo "formatting SUN0535"
        cat /util/format.input.SUN0535 | \
		/usr/sbin/format ${drive} >/dev/null 2>&1
        ;;
    SUN1.05)
       echo "formatting SUN1.05"
       cat /util/format.input.SUN1.05 | \
		/usr/sbin/format ${drive} >/dev/null 2>&1
       ;;
    SUN2.1G)
       echo "formatting SUN2.1G"
       cat /util/format.input.SUN2.1G | \
		/usr/sbin/format ${drive} >/dev/null 2>&1
       ;;
    SEAGATE*5660N)
       echo "formatting SEAGATE-ST5660N"
       cat /util/format.input.seagate.5660 | \
		/usr/sbin/format ${drive} >/dev/null 2>&1
       ;;
    *)
       echo "can't format the disks - type unknown"
       echo "Call an expert !"
       exit 1
       esac
esac



echo "Making the filesystems..."
echo ""
echo "Making the root file system"
echo "y" | /usr/sbin/newfs -v  $rrootdrive

echo "Making the usr file system"
echo "y" | /usr/sbin/newfs -v  $rusrdrive

echo " Making the cache file system "
echo "y" | /usr/sbin/newfs -v $rcachedrive

echo "Making the var filesystem"
echo "y" | /usr/sbin/newfs -v $rvardrive


echo "Adding AFS filesystem"
echo "Making an AFS cache available"
mkdir /var/usr
mkdir /var/usr/vice; 
mount $cachedrive  /var/usr/vice
cd /var/usr/vice
mkdir etc; mkdir cache;
chmod 0700 cache
for i in cacheinfo CellServDB SuidCells ThisCell
        do cp -p /afsin/$i etc/ ; done
cd etc
mv CellServDB CellServDB.public
ln -s CellServDB.public CellServDB
cp -p SuidCells SuidCells.public

echo "Making an /afs repository"
mkdir /tmp/afs
echo "Loading afs in the kernel"
modload /kernel/fs/afs
echo "Starting afsd "
/etc/afsd -nosettime -daemons 4

sh /srvd/install/phase3.sh
