#!/sbin/sh

CONFVARS=$UPDATE_ROOT/var/athena/update.confvars
. $CONFVARS

# If we have one partition of sufficient size, install all packages
# local and use the unmodified patches.  If we have many partitions or
# the one partition is smallish, install the packages from the non-local
# package list as symlinks and use the adjusted patches.
usrpart=`df -k $UPDATE_ROOT/usr | awk '{part=$NF;} END {print part;}'`
rootsize=`df -k ${UPDATE_ROOT:-/} | awk '{sz=$2;} END {print sz;}'`
if [ "$usrpart" = ${UPDATE_ROOT:-/} -a 3145728 -le "$rootsize" ]; then
  echo "Installing packages and srvd local."
  cdnl=/install/cdrom
  patches=/install/patches
  subscr=sys_rvd.local
else
  cdnl=/install/cdrom/cdrom.link
  patches=/install/patches/patches.link
  subscr=sys_rvd
fi

# drvconfig requires this.  In theory, nothing else should.
cd ${UPDATE_ROOT:-/}

if [ "$NEWOS" = "true" ]; then
  echo "Removing the patch/pkg DB for a new OS major rev"
  pks="`cat $OLDPKGS`"
  for i in $pks; do
    rm -rf $UPDATE_ROOT/var/sadm/pkg/$i
  done
  patches="`cat $OLDPTCHS`"
  for i in $patches; do
    rm -rf $UPDATE_ROOT/var/sadm/patch/$i
  done
fi

if [ -s "$DEADFILES" ]; then
  echo "Removing outdated files"
  dead="`cat $DEADFILES`"
  for i in $dead; do
    rm -rf $UPDATE_ROOT/$i
  done
fi

yes="y\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny\ny"

if [ -s "$LOCALPACKAGES" -o -s "$LINKPACKAGES" ]; then
  pkglog=$UPDATE_ROOT/var/athena/update.pkglog
  rm -f "$pkglog"
fi

if [ -s "$LOCALPACKAGES" ]; then
  echo "Installing os local packages"
  for i in `cat "$LOCALPACKAGES"`; do
    echo "$i"
    echo "$yes" | pkgadd -R "$UPDATE_ROOT" -d /install/cdrom "$i"
  done 2>>$pkglog
fi

if [ -s "$LINKPACKAGES" ]; then
  echo "Installing the os link packages"
  for i in `cat "$LINKPACKAGES"`; do
    echo "$i"
    echo "$yes" | pkgadd -R "$UPDATE_ROOT" -d "$cdnl" "$i"
  done 2>>$pkglog
fi

if [ "$NEWOS" = "true" ]; then
  echo "Making adjustments"
  cp /cdrom/I* "$UPDATE_ROOT/var/sadm/system/admin"
  rm $UPDATE_ROOT/etc/.UNC*
  rm "$UPDATE_ROOT/etc/.sysidconfig.apps"
  cp /cdrom/.sysIDtool.state "$UPDATE_ROOT/etc/default"
fi

if [ -s "$PATCHES" ]; then
  echo "Installing OS patches"
  # patchadd is stupid and elides blank arguments, so we have to be careful
  # specifying the update root.
  ur="${UPDATE_ROOT:+-R $UPDATE_ROOT}"
  echo "$yes" | patchadd -d $ur -u -M "$patches" `cat $PATCHES`
fi

if [ "$OSCHANGES" = true ]; then
  echo "Performing local OS changes"
  sh /srvd/install/oschanges

  # Restore any config file that pkgadd has replaced.
  echo "Re-copying config files after OS update"
  if [ -s "$CONFCHG" ]; then
    for i in `cat $CONFCHG`; do
      if [ -f "/srvd$i" ]; then
	rm -rf "$UPDATE_ROOT$i"
	cp -p "/srvd$i" "$UPDATE_ROOT$i"
      fi
      if [ true = "$PUBLIC" ]; then
	rm -rf "$UPDATE_ROOT$i.saved"
      fi
    done
  fi
fi

# Force a device reconfigure on reboot.
touch $UPDATE_ROOT/reconfigure

echo "Finished os installation"

echo "Tracking the srvd"
track -d -F /srvd -T "$UPDATE_ROOT" -W /srvd/usr/athena/lib \
  -s stats/$subscr slists/$subscr
rm -f $UPDATE_ROOT/var/athena/rc.conf.sync

echo "Copying kernel modules from /srvd/kernel"
cp -p -r /srvd/kernel/fs/* "$UPDATE_ROOT/kernel/fs"

rm -f "$UPDATE_ROOT/var/spool/cron/crontabs/uucp"

if [ "$NEWDEVS" = "true" ]; then
  echo "Create devices and dev"
  drvconfig -R "$UPDATE_ROOT" -r devices -p "$UPDATE_ROOT/etc/path_to_inst"
  chmod 755 "$UPDATE_ROOT/dev"
  chmod 755 "$UPDATE_ROOT/devices"
  cp /dev/null "$UPDATE_ROOT/reconfigure"
fi
