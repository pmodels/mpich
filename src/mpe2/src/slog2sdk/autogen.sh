#! /bin/sh
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOCONF=${AUTOCONF:-autoconf}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
MPE_AUTOHEADER=${MPE_AUTOHEADER:-$AUTOHEADER}
MPE_AUTOCONF=${MPE_AUTOCONF:-$AUTOCONF}
MPE_LIBTOOLIZE=${MPE_LIBTOOLIZE:-$LIBTOOLIZE}
SLOG2_AUTOHEADER=${SLOG2_AUTOHEADER:-$MPE_AUTOHEADER}
SLOG2_AUTOCONF=${SLOG2_AUTOCONF:-$MPE_AUTOCONF}
SLOG2_LIBTOOLIZE=${SLOG2_LIBTOOLIZE:-$MPE_LIBTOOLIZE}

# Check that we have a workable autoconf
acWorks=no
if test -d .tmp ; then rm -rf .tmp ; fi
if test -s .tmp ; then rm -f .tmp ; fi
if test ! -d .tmp ; then
    mkdir .tmp 2>&1 >/dev/null
fi
rm -f .tmp/configure.in .tmp/configure
cat >.tmp/configure.in <<EOF
AC_PREREQ(2.52)
EOF
if (cd .tmp && $SLOG2_AUTOCONF >/dev/null 2>&1 ) ; then
    acWorks=yes
fi
if [ "$acWorks" != yes ] ; then
    echo "Selected version of autoconf cannot handle version 2.52"
    echo "Trying to find an autoconf-2.xx..."
    acver_min=59
    acver_max=69
    ver="$acver_max"
    while [ "$ver" -ge "$acver_min" ] ; do
        autoconf="autoconf-2.$ver"
        if (cd .tmp && $autoconf >/dev/null 2>&1 ) ; then
            SLOG2_AUTOCONF=$autoconf
            SLOG2_AUTOHEADER="autoheader-2.$ver"
            echo "Found $autoconf"
            acWorks=yes
            break
        fi
        ver="`expr $ver - 1`"
    done
    if [ "$acWorks" != yes ] ; then
        echo "Unable to find workable autoconf"
        exit 1
    fi
fi
rm -rf .tmp

# The parent directory of where this script is located
pgmdir="`dirname $0`"
master_dir=`(cd $pgmdir && pwd)`
cfgins=`find $master_dir -name 'configure.in' -print`

# Locate all configure.ins that need libtoolize.
for cfgin in $cfgins ; do
    dir="`dirname $cfgin`"
    if [ -n "`grep AC_PROG_LIBTOOL $cfgin`" ] ; then
        echo "Running libtoolize in $dir/ ..."
        (cd $dir && $SLOG2_LIBTOOLIZE -ifc) || exit 1
    fi
done

# Locate all the configure.in under master_dir to invoke autoconf.
for cfgin in $cfgins ; do
    dir="`dirname $cfgin`"
    if [ -n "`grep AC_CONFIG_HEADER $cfgin`" ] ; then
        echo "Running autoheader/autoconf in $dir/ ..."
        (cd $dir && $SLOG2_AUTOHEADER && $SLOG2_AUTOCONF && rm -rf autom4te*.cache) || exit 1
    else
        echo "Running autoconf in $dir/ ..."
        (cd $dir && $SLOG2_AUTOCONF && rm -rf autom4te*.cache) || exit 1
    fi
done
