#!/bin/sh
#
# Update script for Project Athena workstations
#
# Check that an update is needed, and make sure the conditions necessary
# for a successful update are met. Then prepare the machine for update,
# and run do_update.
#
# $Id: update_ws.sh,v 1.7 1996-06-08 00:02:03 ghudson Exp $
#

trap "" 1 15

ROOT="${ROOT-}"; export ROOT
CONFDIR="${CONFDIR-/etc/athena}"; export CONFDIR
LIBDIR=/srvd/usr/athena/lib/update;	export LIBDIR
PATH=/bin:/srvd/bin:/srvd/etc:/srvd/etc/athena:/srvd/bin/athena:/srvd/usr/athena/etc:/srvd/usr/bin:/srvd/usr/etc:/os/usr/ucb:/usr/bin:/usr/ucb:$LIBDIR ; export PATH

RC=nothing
HOSTNAME=hostname
WHOAMI=whoami
LOGGER=logger

case `/bin/athena/machtype` in
sgi)
	RC=rc
	CONFIG=/etc/config
	HOSTNAME=/usr/bsd/hostname
	WHOAMI=/bin/whoami
	LOGGER=/usr/bsd/logger
	;;
sun4)
	RC=rc
	;;
esac

#
# If /srvd is not mounted, quit.
#
if [ ! -d /srvd/bin ]; then
	exit 1
fi

if [ ! -f ${ROOT}/${CONFDIR}/version -a -f ${ROOT}/etc/version ]; then
	CONFDIR=/etc; export CONFDIR;
fi

if [ ! -f ${ROOT}/${CONFDIR}/version ]; then
	echo "Athena Update (???) Version 0.0A Mon Jan 1 00:00:00 EDT 0000" \
		> ${ROOT}/${CONFDIR}/version
fi

NEWVERS=`awk '{a=$5}; END{print a}' /srvd/.rvdinfo`
VERSION=`awk '{a=$5}; END{print a}' ${ROOT}/${CONFDIR}/version`
export NEWVERS VERSION

if [ -f ${ROOT}/${CONFDIR}/rc.conf ]; then
   . ${ROOT}/${CONFDIR}/rc.conf
else
   PUBLIC=true; export PUBLIC
   AUTOUPDATE=true; export AUTOUPDATE
fi

AUTO=false
if [ `expr $0 : '.*auto_update'` != "0" ]; then
	AUTO=true;
fi

/etc/athena/save_cluster_info
if [ ! -f /etc/athena/clusterinfo.bsh ]; then
	# No updates for machines without cluster info.
	if [ "$AUTO" = false ]; then
		echo "Cannot find Hesiod information for this machine, aborting update."
	fi
	exit 1
fi
. /etc/athena/clusterinfo.bsh

case `echo $VERSION $NEWVERS | awk '(NR==1) { \
	if ($1 == "Update") {print "OOPS"} \
	else if ($1 >= $2) {print "OK"} \
	else if (substr($1,1,3) == substr($2,1,3)) {print "INCR"} \
	else {print "FULL"}}'` in
OOPS)
	if [ ! -f /usr/tmp/update.check -a -f $LOGGER ]; then
		$LOGGER -t `$HOSTNAME` -p user.notice at revision $VERSION
		cp /dev/null /usr/tmp/update.check
	fi

	echo "This system is in the middle of an update.  Please contact"
	echo "Athena Operations at x3-1410. Thank you. -Athena Operations"
	exit 1
	;;
OK)
	if [ "${AUTO}" != "true" ]; then
		# User ran update_ws; display something appropriate.
		if [ -n "$NEW_PRODUCTION_RELEASE" -o \
		     -n "$NEW_TESTING_RELEASE" ]; then
			echo "Your workstation software already matches the"
			echo "version on the system packs.  You must manually"
			echo "attach a newer version of the system packs to"
			echo "update beyond this point."
		else
			echo "It appears you already have this update."
		fi
	else
		# System ran auto_update; point out new releases if available.
		if [ -n "$NEW_PRODUCTION_RELEASE" ]; then
			/bin/cat <<EOF

A new Athena release ($NEW_PRODUCTION_RELEASE) is available.  Since it may be
incompatible with your workstation software, your workstation
is still using the old system packs.  Please contact Athena
Hotline (x3-1410) to have your workstation updated.
EOF
		fi
		if [ -n "$NEW_TESTING_RELEASE" ]; then
			/bin/cat << EOF

A new Athena release ($NEW_TESTING_RELEASE) is now in testing.  You are
theoretically interested in this phase of testing, but
because there may be bugs which would inconvenience
your work, you must update to this release manually.
Please contact Athena Hotline (x3-1410) if you have
not received instructions on how to do so.
EOF
		fi
	fi
	exit 0
	;;
INCR|FULL)
	;;
esac

if [ "${PUBLIC}" = "true" ]; then
	AUTOUPDATE=true
fi

if [ "${AUTO}" = "true" -a "${AUTOUPDATE}" != "true" ]; then
	cat <<EOF

	A new version of Athena software is now available.
	Please contact Athena Operations to get more information
	on how to update your workstation yourself, or to schedule
	us to do it for you. Thank you.  -Athena Operations
EOF
	if [ ! -f /usr/tmp/update.check -a -f $LOGGER ]; then
		$LOGGER -t `$HOSTNAME` -p user.notice at revision $VERSION
		cp /dev/null /usr/tmp/update.check
	fi

	case "$1" in
	reactivate|rc|"") ;;
	*) sleep 20 ;;
	esac

	exit 1
fi

# Version-specific updating (forcing compatibility, if necessary)
case $VERSION in
7.3*)	;;
*)
	# For an update to succeed on the DECstation, svc.conf must be present
	# or the whoami will fail
	if [ -f /srvd/etc/svc.conf -a ! -f /etc/svc.conf ]; then
        	/srvd/bin/cp -p /srvd/etc/svc.conf /etc/svc.conf
	fi
	;;
esac

if [ "`$WHOAMI`" != "root" ];  then
	echo "You are not root.  This update script must be run as root."
	exit 1
fi

SITE=/var

if [ -d ${SITE}/server ] ; then
	# shutdown kills off named which we need.
	case `/bin/athena/machtype` in
	rsaix)
		/etc/athena/named
		;;
	sun*)
		/usr/sbin/in.named /etc/named.boot
		;;
	sgi)
		/usr/sbin/named `cat $CONFIG/named.options 2> /dev/null` < /dev/null
		;;
	*)
		/etc/named /etc/named.boot
		;;
	esac

	/bin/athena/attach -q -h -n -o hard mkserv
	MKSERV=`/usr/athena/bin/athdir /mit/mkserv`/mkserv
	if [ -s ${MKSERV} ]; then
		export MKSERV
		${MKSERV} updatetest
		if [ $? -ne 0 ]; then
			echo "Update cannot be performed as mkserv services cannot be found for all services"
			exit 1
		fi
	else
		echo "Update cannot be performed as mkserv binary not available"
		exit 1
	fi
fi

if [ `/bin/athena/machtype` = sgi ]; then
	if [ ! -n "$INSTLIB" ]; then
		echo "No installation library set in Hesiod information, aborting update."
		exit 1
	fi

	/bin/athena/attach -q -h -n -o hard $INSTLIB
	if [ ! -d /install/install ]; then
		echo "Installation directory can't be found, aborting update."
		exit 1
	fi
fi

if [ ! -d ${ROOT}/.deleted ] ; then
	mkdir ${ROOT}/.deleted
fi

if [ "${AUTO}" = "true" -a "$1" = "reactivate" ]; then
	if [ -f /etc/athena/dm.pid ]; then
		kill -FPE `cat /etc/athena/dm.pid`
		ln ${ROOT}/etc/athena/dm ${ROOT}/.deleted/
	fi

	if [ -f /etc/init.d/axdm ]; then
		/etc/init.d/axdm stop
	fi

	sleep 2
	if [ `/bin/athena/machtype` = sgi ]; then
		exec 1>/dev/tport 2>&1
	fi
fi

FULLCOPY=true; export FULLCOPY

if [ ${AUTO}x = "truex" ]; then
	cat <<EOF

THIS WORKSTATION IS ABOUT TO UNDERGO AN AUTOMATIC SOFTWARE UPDATE.
THIS PROCEDURE MAY TAKE SOME TIME.

PLEASE DO *NOT* DISTURB IT WHILE THIS IS IN PROGRESS.

EOF
	case "$1" in
	${RC}|reactivate|activate|deactivate)
		case `/bin/athena/machtype` in
		sgi)
			/etc/killall -TERM inetd snmpd zhm syslogd
			;;
		*)
		        rm /tmp/pids
		        cat /etc/inetd.pid >/tmp/pids
		        cat /etc/athena/inetd.pid >>/tmp/pids
		        cat /etc/snmpd.pid >>/tmp/pids
		        cat /etc/athena/snmpd.pid >>/tmp/pids
		        cat /etc/athena/zhm.pid >>/tmp/pids
		        cat /etc/syslog.pid >>/tmp/pids
			(kill -TERM `cat /tmp/pids`) > /dev/null 2>&1
			;;
		esac
		;;
	esac

	/bin/sh $LIBDIR/do_update < /dev/null
	/bin/athena/detach -h -n -q -a
	/bin/sync
	case "${SYSTEM}" in
ULTRIX*)
		echo "Auto Update Done, System will now reboot."
		/bin/sync
		exec /etc/reboot
		;;
*)
		echo "Auto Update Done, System will reboot in 15 seconds."
		/bin/sync
		/bin/sleep 15
		exec /etc/reboot -q
		;;
	esac
else
	/bin/sh $LIBDIR/do_update
fi
echo "Done"
exit 0
