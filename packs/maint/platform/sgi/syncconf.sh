#!/bin/sh
# $Id: syncconf.sh,v 1.2 1997-02-11 18:26:48 ghudson Exp $

config=/etc/config
setconfig="/sbin/chkconfig -f"
rcconf=/etc/athena/rc.conf
rcsync=/etc/athena/.rc.conf.sync
rcsyncout=$rcsync
handled=
rc2added=
all=
debug=
startup=
echo=echo
maybe=

configbool()
{
	$maybe $setconfig "$1" "$2"
}

configopt()
{
	if [ -n "$debug" ]; then
		$echo "echo $2 > $config/$1"
	else
		echo "$2" > "$config/$1"
	fi
}

syncvar()
{
	case "$2" in
	true|on)
		configbool "$1" on
		;;
	false|off)
		configbool "$1" off
		;;
	*)
		configopt "$1" "$2"
		;;
	esac
}

# Usage: syncrc2 scriptname order value
# e.g. "syncrc2 mail 50 false" turns off the /etc/rc2.d/S50mail link.
syncrc2()
{
	if [ "$3" = false ]; then
		prefix=s
	else
		prefix=S
	fi
	if [ "$3" != false -a ! -h "/etc/rc2.d/S$2$1" ]; then
		rc2added=1
	fi
	$maybe rm -f "/etc/rc2.d/s$2$1" "/etc/rc2.d/S$2$1"
	$maybe ln -s "../init.d/$2" "/etc/rc2.d/$prefix$2$1"
}

# Usage: syncrc scriptname order value
# e.g. "syncrc0 mail 20 true" turns on the /etc/rc2.d/K20mail link.
syncrc0()
{
	if [ "$3" = false ]; then
		prefix=k
	else
		prefix=K
	fi
	$maybe rm -f "/etc/rc0.d/k$2$1" "/etc/rc0.d/K$2$1"
	$maybe ln -s "../init.d/$2" "/etc/rc0.d/$prefix$2$1"
}

remove()
{
	$maybe rm -f "$1"
}

move()
{
	$maybe mv "$1" "$2"
}

quiet_move()
{
	if [ -f "$1" ]; then
		move "$1" "$2"
	fi
}

put()
{
	if [ -n "$debug" ]; then
		$echo "echo $2 > $1"
	else
		echo "$2" > "$1"
	fi
}

append()
{
	if [ -n "$debug" ]; then
		$echo "echo $2 >> $1"
	else
		echo "$2" >> "$1"
	fi
}

handle()
{
	# Don't handle anything twice.
	case "$handled" in
	*" $1 "*)
		return
		;;
	esac
	handled="$handled $1 "

	case "$1" in
	TIMECLIENT)
		# gettime and AFS time synchronization
		syncvar timeclient "$TIMECLIENT"
		dependencies="$dependencies AFSCLIENT"
		;;

	TIMEHUB)
		syncvar timeclient.options "$TIMEHUB"
		;;

	TIMESRV)
		syncvar timed "$TIMESRV"
		;;

	AFSCLIENT)
		if [ "$AFSCLIENT" != false ]; then

			syncvar afsml true
			syncvar afsclient true

			afsclientnum=4
			if [ "$AFSCLIENT" != "true" ]; then
				afsclientnum="$AFSCLIENT"
			fi

			options="-stat 2000 -dcache 800 -volumes 70"
			options="$options -daemons $afsclientnum"
			if [ "$TIMECLIENT" = false ]; then
				options="$options -nosettime"
			fi

			syncvar afsd.options "$options"
		else
			syncvar afsml false
			syncvar afsclient false
		fi
		;;

	NFS)
		if [ "$NFSSRV" != false -o "$NFSCLIENT" != false ]; then
			syncvar nfs true

		        case "$NFSCLIENT" in
			true|false)
				remove $config/biod.options
				;;
			*)
				syncvar biod.options "$NFSCLIENT"
				;;
			esac

			case "$NFSSRV" in
	                true)
				remove $config/nfsd.options
				;;
			false)
				syncvar nfsd.options 0
				;;
			*)
				syncvar nfsd.options "$NFSSRV"
				;;
			esac
		else
			syncvar nfs false
		fi
		;;

	SENDMAIL)
		syncrc2 mail 50 "$SENDMAIL"
		syncrc0 mail 20 "$SENDMAIL"
		;;

	SNMP)
		syncvar snmpd "$SNMP"
		;;

	SAVECORE)
		if [ "$SAVECORE" != false ]; then
			case "$SAVECORE" in
			true)
				minfree=30000000
				;;
			*)
				minfree="$SAVECORE"
				;;
			esac
			put /var/adm/crash/minfree $minfree
		fi
		syncrc2 savecore 48 "$SAVECORE"
		;;

	ACCOUNT)
		syncvar acct "$ACCOUNT"
		;;

	QUOTAS)
		syncvar quotacheck "$QUOTAS"
		syncvar quotas "$QUOTAS"
		;;

	HOSTADDR)
		move /etc/sys_id /etc/sys_id.saved
		move /etc/hosts /etc/hosts.saved
		move $config/staticroute.options \
			$config/staticroute.options.saved
		move $config/ifconfig-1.options \
			$config/ifconfig-1.options.saved

		net=`echo $ADDR | awk -F. '{ print $1 "." $2 }'`
		gateway=$net.0.1
		broadcast=$net.255.255

		put    /etc/sys_id $HOST
		put    $config/staticroute.options $gateway
		put    $config/ifconfig-1.options "netmask 0xffff0000"
		append $config/ifconfig-1.options "broadcast $broadcast"
		put    /etc/hosts "#"
		append /etc/hosts "# Internet host table"
		append /etc/hosts "#"
		append /etc/hosts "127.0.0.1  localhost"
		append /etc/hosts "$ADDR  $HOST.MIT.EDU $HOST"
		;;

	*)
		$echo "syncconf: unknown variable $1"
		;;
	esac
}

while getopts anq opt; do
	case "$opt" in
	a)
		all=1
		;;
	n)
		debug=1
		rcsyncout=/tmp/rc.conf.sync
		maybe=$echo
		;;
	q)
		echo=:
		;;
	\?)
		echo "Usage: syncconf [-anq]"
		exit 1
		;;
	esac
done
shift `expr $OPTIND - 1`
if [ "$#" -ne 0 ]; then
	echo "Usage: syncconf [-anq]"
	exit 1
fi

$echo "Synchronizing configuration... \c"

. "$rcconf"

if [ -z "$all" -a -f "$rcsync" ]; then
	. "$rcsync"
else
	changes="TIMECLIENT TIMEHUB TIMESRV AFSCLIENT NFS SENDMAIL SNMP"
	changes="$changes SAVECORE ACCOUNT QUOTAS HOSTADDR"
fi

if [ -z "$changes" ]; then
	$echo "No changes to synchronize."
	exit
fi

for i in $changes; do
	$echo "$i \c"
	if [ -n "$debug" ]; then
		$echo ""
	fi
	handle "$i"
done

for i in $dependencies; do
	$echo "($i) \c"
	if [ -n "$debug" ]; then
		$echo ""
	fi
	handle "$i"
done

$echo ""

cat > $rcsyncout << EOF
if [ \$TIMECLIENT != $TIMECLIENT ]; then changes="\$changes TIMECLIENT"; fi
if [ \$TIMEHUB != $TIMEHUB ]; then changes="\$changes TIMEHUB"; fi
if [ \$TIMESRV != $TIMESRV ]; then changes="\$changes TIMESRV"; fi
if [ \$AFSCLIENT != $AFSCLIENT ]; then changes="\$changes AFSCLIENT"; fi
if [ \$NFSSRV != $NFSSRV ]; then changes="\$changes NFS"; fi
if [ \$NFSCLIENT != $NFSCLIENT ]; then changes="\$changes NFS"; fi
if [ \$SENDMAIL != $SENDMAIL ]; then changes="\$changes SENDMAIL"; fi
if [ \$SNMP != $SNMP ]; then changes="\$changes SNMP"; fi
if [ \$SAVECORE != $SAVECORE ]; then changes="\$changes SAVECORE"; fi
if [ \$ACCOUNT != $ACCOUNT ]; then changes="\$changes ACCOUNT"; fi
if [ \$QUOTAS != $QUOTAS ]; then changes="\$changes QUOTAS"; fi
if [ \$HOST != $HOST ]; then changes="\$changes HOSTADDR"; fi
if [ \$ADDR != $ADDR ]; then changes="\$changes HOSTADDR"; fi
EOF

if [ -n "$rc2added" ]; then
	# Exit with status 1 to indicate need to reboot if run at startup.
	exit 1
fi
