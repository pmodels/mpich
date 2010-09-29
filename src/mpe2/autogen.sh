#! /bin/sh
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOCONF=${AUTOCONF:-autoconf}
MPE_AUTOHEADER=${MPE_AUTOHEADER:-$AUTOHEADER}
MPE_AUTOCONF=${MPE_AUTOCONF:-$AUTOCONF}

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
if (cd .tmp && $MPE_AUTOCONF >/dev/null 2>&1 ) ; then
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
	    MPE_AUTOCONF=$autoconf
	    MPE_AUTOHEADER="autoheader-2.$ver"
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

# We cannot use "find . -name 'configre.in'" to locate configure.ins
# because "." is the working directory not the top-level MPE2 source
# directory.  So we use the full-pathname of this script to locate the
# MPE2's top-level directory.  'dirname' does not return the full-pathname
# and may not be reliable in general, so we cd to the directory and
# use pwd to get the full-pathname of the top-level MPE2 source directory.
pgmdir="`dirname $0`"
master_dir=`(cd $pgmdir && pwd)`

# Remove all the configure first so we can track if the autoconf has been run.
find $master_dir -name 'configure' -exec rm -f {} \;

# Locate all the autogen.sh and invoke the script accordingly.
atgs=`find $master_dir -name 'autogen.sh' -print`
for atg in $atgs ; do
    dir="`dirname $atg`"
    if [ "$atg" != "$master_dir/autogen.sh" ] ; then
        echo "Running autogen.sh in $dir/ ..."
        $atg || exit 1
    fi
done

# Locate all the configure.in and invoke autoconf if no configure is found.
cfgins=`find $master_dir -name 'configure.in' -print`
for cfgin in $cfgins ; do
    dir="`dirname $cfgin`"
    if [ ! -x "$dir/configure" ] ; then
        if [ -n "`grep AC_CONFIG_HEADER $cfgin`" ] ; then
            echo "Running autoheader/autoconf in $dir/ ..."
            (cd $dir && $MPE_AUTOHEADER && $MPE_AUTOCONF && rm -rf autom4te*.cache) || exit 1
        else
            echo "Running autoconf in $dir/ ..."
            (cd $dir && $MPE_AUTOCONF && rm -rf autom4te*.cache) || exit 1
        fi
    fi
done
