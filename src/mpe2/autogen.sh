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
    for ver in 56 55 54 53 52 ; do
        autoconf="autoconf-2.$ver"
        if (cd .tmp && $autoconf >/dev/null 2>&1 ) ; then
	    MPE_AUTOCONF=$autoconf
	    MPE_AUTOHEADER="autoheader-2.$ver"
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

cfgins=`find . -name 'configure.in' -print`
for cfgin in $cfgins ; do
    dir="`dirname $cfgin`"
    echo "Building directory $dir"
    if [ ! -z "`grep AC_CONFIG_HEADS $cfgin`" ] ; then
	(cd $dir && $MPE_AUTOHEADER && $MPE_AUTOCONF && rm -rf autom4te*.cache) || exit 1
    else
	(cd $dir && $MPE_AUTOCONF && rm -rf autom4te*.cache) || exit 1
    fi
done
