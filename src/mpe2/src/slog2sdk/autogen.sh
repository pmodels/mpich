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
    for ver in `seq 52 69 | sort -r` ; do
        autoconf="autoconf-2.$ver"
        if (cd .tmp && $autoconf >/dev/null 2>&1 ) ; then
	    SLOG2_AUTOCONF=$autoconf
	    SLOG2_AUTOHEADER="autoheader-2.$ver"
	    echo "Found $autoconf"
	    acWorks=yes
	    break
        fi
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
echo "master_dir = $master_dir"

# *** FIXME ****
# Given the mpich2/maint/updatefiles uses libtoolize, this script should be
# updated to call LIBTOOLIZE -ivfc to update all the libtool related files.
# ALso, mpe2/autogen.sh should not do autoheader/autoconf on slog2sdk and
# should instead call slog2sdk/maint/updatefiles.


# Locate all the configure.in under master_dir
cfgins=`find $master_dir -name 'configure.in' -print`
for cfgin in $cfgins ; do
    dir="`dirname $cfgin`"
    if [ -n "`grep AC_PROG_LIBTOOL $cfgin`" ] ; then
        echo "Running libtoolize in $dir/ ..."
        (cd $dir && $SLOG2_LIBTOOLIZE -ivfc) || exit 1
    fi
    echo "Creating configure in $dir/ ..."
    if [ -n "`grep AC_CONFIG_HEADER $cfgin`" ] ; then
        (cd $dir && $SLOG2_AUTOHEADER && $SLOG2_AUTOCONF && rm -rf autom4te*.cache) || exit 1
    else
        (cd $dir && $SLOG2_AUTOCONF && rm -rf autom4te*.cache) || exit 1
    fi
done
