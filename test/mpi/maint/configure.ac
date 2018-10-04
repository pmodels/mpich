# -*- Mode: shell-script; -*-
AC_PREREQ(2.59)
AC_INIT
#
# Try to find the version
if test -s "$srcdir/Version" ; then
    VERSION="`cat $srcdir/Version`"
else
    VERSION="Unknown"
fi
AC_SUBST(VERSION)
AC_PATH_PROG(PERL,perl)

# Find the location of the mpich sources
if test -z "$abs_srcdir" ; then
    abs_srcdir=`cd $srcdir && pwd`
fi
abs_mpichsrcdir=`cd $abs_srcdir && cd .. && pwd`

AC_SUBST(abs_mpichsrcdir)

# Does xargs need the -r option to handle the case where the input 
# is empty (gnu utils do, Mac OSX does not accept -r)
xargs_out=`echo "" | xargs ls | wc -l | sed -e 's/ //g'`
if test "$xargs_out" != "0" ; then
    XARGS_NODATA_OPT=-r
fi
AC_SUBST(XARGS_NODATA_OPT)
