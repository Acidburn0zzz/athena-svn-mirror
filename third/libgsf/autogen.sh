#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="libgsf"

(test -f $srcdir/configure.in \
  && test -d $srcdir/gsf \
  && test -f $srcdir/gsf/gsf.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level libgsf directory"
    exit 1
}

ifs_save="$IFS"; IFS=":"
for dir in $PATH ; do
  test -z "$dir" && dir=.
  if test -f $dir/gnome-autogen.sh ; then
    gnome_autogen="$dir/gnome-autogen.sh"
    gnome_datadir=`echo $dir | sed -e 's,/bin$,/share,'`
    break
  fi
done
IFS="$ifs_save"

if test -z "$gnome_autogen" ; then
  echo "You need to install the gnome-common module and make"
  echo "sure the gnome-autogen.sh script is in your \$PATH."
  exit 1
fi

GNOME_DATADIR="$gnome_datadir" USE_GNOME2_MACROS=1 . $gnome_autogen