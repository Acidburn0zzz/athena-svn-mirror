#!/bin/sh
# $Id: syncconf.sh,v 1.3.2.2 2000-08-23 19:53:06 ghudson Exp $

rcconf=/etc/athena/rc.conf
rcsync=/var/athena/rc.conf.sync
rcsyncout=$rcsync
handled=
all=
debug=
startup=
echo=echo
maybe=


remove()
{
  $maybe rm -f "$1"
}

move()
{
  $maybe mv -f "$1" "$2"
}

put()
{
  if [ -n "$debug" ]; then
    echo "echo $2 > $1"
  else
    echo "$2" > "$1"
  fi
}

append()
{
  if [ -n "$debug" ]; then
    echo "echo $2 >> $1"
  else
    echo "$2" >> "$1"
  fi
}

update()
{
  $maybe ln -f "$1" "$1.saved"
  $maybe /bin/athena/syncupdate "$1.new" "$1"
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
  HOSTADDR)
    remove /etc/sysconfig/network.new
    remove /etc/hosts.new
    remove /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new

    append /etc/hosts.new "127.0.0.1	localhost"

    append /etc/sysconfig/network.new "NETWORKING=yes"
    append /etc/sysconfig/network.new "FORWARD_IPV4=false"

    append /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new "DEVICE=$NETDEV"
    append /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new "ONBOOT=yes"

    if [ "$ADDR" = dhcp ]; then
      append /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new \
	"BOOTPROTO=dhcp"
    else
      set -- `/etc/athena/netparams $ADDR`
      netmask=$1
      network=$2
      broadcast=$3
      gateway=$4

      append /etc/hosts.new "$ADDR      $HOST"
	
      append /etc/sysconfig/network.new "HOSTNAME=$HOST"
      append /etc/sysconfig/network.new "GATEWAY=$gateway"

      append /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new \
	"BOOTPROTO=static"
      append /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new "IPADDR=$ADDR"
      append /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new \
	"NETMASK=$netmask"
      append /etc/sysconfig/network-scripts/ifcfg-$NETDEV.new \
	"NETWORK=$network"
    fi

    update /etc/hosts
    update /etc/sysconfig/network
    update /etc/sysconfig/network-scripts/ifcfg-$NETDEV

    # We run before networking is set up, but after the hostname is set.
    # We're pretty sure we run before anything actually uses the
    # hostname.  Reset the hostname in case it changed.
    hostname "$HOST"

    ;;

  MAILRELAY)
    case $MAILRELAY in
    none)
      remove /etc/athena/sendmail.conf
      ;;
    default)
      case $ADDR,$HOST in
      dhcp,*)
        remove /etc/athena/sendmail.conf
	;;
      *,*.MIT.EDU|*,*.mit.edu)
	put /etc/athena/sendmail.conf "relay ATHENA.MIT.EDU"
	;;
      *,*)
	remove /etc/athena/sendmail.conf
	;;
      esac
      ;;
    *)
      put /etc/athena/sendmail.conf "relay $MAILRELAY"
      ;;
    esac
    ;;

  AFS)
    remove /etc/sysconfig/afs.new

    append /etc/sysconfig/afs.new "#! /bin/sh"

    if [ "true" = "$AFSSRV" ]; then
      append /etc/sysconfig/afs.new "AFS_SERVER=on"
    else
      append /etc/sysconfig/afs.new "AFS_SERVER=off"
    fi

    if [ "true" = "$AFSCLIENT" ]; then
      append /etc/sysconfig/afs.new "AFS_CLIENT=on"
    else
      append /etc/sysconfig/afs.new "AFS_CLIENT=off"
    fi
    
    update /etc/sysconfig/afs
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

$echo -n "Synchronizing configuration... "

. "$rcconf"

if [ -z "$all" -a -f "$rcsync" ]; then
  . "$rcsync"
else
  changes="HOSTADDR MAILRELAY AFS"
fi

if [ -z "$changes" ]; then
  $echo "No changes to synchronize."
  exit
fi

for i in $changes; do
  $echo -n "$i "
  if [ -n "$debug" ]; then
    $echo ""
  fi
  handle "$i"
done

for i in $dependencies; do
  $echo -n "($i) "
  if [ -n "$debug" ]; then
    $echo ""
  fi
  handle "$i"
done

$echo ""

cat > $rcsyncout << EOF
# This file was generated by /etc/athena/syncconf; do not edit.
if [ \$HOST != $HOST ]; then changes="\$changes HOSTADDR MAILRELAY"; fi
if [ \$ADDR != $ADDR ]; then changes="\$changes HOSTADDR MAILRELAY"; fi
if [ \$MAILRELAY != $MAILRELAY ]; then changes="\$changes MAILRELAY"; fi
if [ \$AFSCLIENT != $AFSCLIENT ]; then changes="\$changes AFS"; fi
if [ \$AFSSRV != $AFSSRV ]; then changes="\$changes AFS"; fi
EOF

if [ -n "$mustreboot" ]; then
  # Exit with status 1 to indicate need to reboot if run at startup.
  exit 1
fi
