##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

dnl Process this file with autoconf to produce a configure script.
dnl
dnl The file name here refers to a file in the source being configured
AC_INIT(rlog.c)

dnl Definitions will be placed in this file rather than in the DEFS variable
AC_CONFIG_HEADER(rlogconf.h)
AH_TOP([/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef FORKERCONF_H_INCLUDED
#define FORKERCONF_H_INCLUDED
])
AH_BOTTOM([#endif])
dnl
AC_CONFIG_AUX_DIR(../../../../confdb)
dnl
echo "RUNNING CONFIGURE FOR RLOG"
dnl
dnl
dnl Enable better caching control
PAC_ARG_CACHING
dnl
AC_ARG_ENABLE(echo, 
[--enable-echo  - Turn on strong echoing. The default is enable=no.] ,set -x)

AC_ARG_ENABLE(sharedlibs,
[--enable-sharedlibs=kind - Enable shared libraries.  kind may be
    gnu     - Standard gcc and GNU ld options for creating shared libraries
    libtool - GNU libtool 
This option is currently ignored])

dnl First check that we have a clean build if we are doing a VPATH build
PAC_VPATH_CHECK()

dnl This test is complicated by the fact that top_srcdir is not set until
dnl the very end of configure.  Instead, we get it ourselves
if test -z "$top_srcdir" ; then
   use_top_srcdir=$srcdir   
else
   use_top_srcdir=$top_srcdir
fi
if test -z "$main_top_srcdir" ; then
    main_top_srcdir=$use_top_srcdir
fi
AC_SUBST(main_top_srcdir)
export main_top_srcdir
#
# Get the main builddir (which may be imported from above)
if test -z "$main_top_builddir" ; then
    main_top_builddir=`pwd`
fi
export main_top_builddir
AC_SUBST(main_top_builddir)

dnl Find a C compiler
AC_PROG_CC
AC_PROG_CC_C99
AC_PATH_PROG(AR,ar)
AC_SUBST(AR_FLAGS)
dnl
dnl AC_PROG_MAKE_SET
AC_PROG_INSTALL
PAC_PROG_CHECK_INSTALL_WORKS
dnl We also need mkdir -p.
PAC_PROG_MKDIR_P
PAC_PROG_MAKE
dnl
AC_PATH_PROG(PERL,perl)
dnl     check for compiler characteristics
dnl PAC_PROG_C_WEAK_SYMBOLS
AC_C_CONST
AC_C_VOLATILE
AC_C_RESTRICT
AC_C_INLINE
dnl PAC_C_STRUCT_ALIGNMENT
dnl
dnl Look for Standard headers
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)
dnl
MPILIBNAME=${MPILIBNAME:-"mpich"}
AC_SUBST(MPILIBNAME)
PMPILIBNAME=${PMPILIBNAME:-"p$MPILIBNAME"}
AC_SUBST(PMPILIBNAME)

dnl
dnl How do I get configure to find these values for me?
dnl use AC_PATH_PROG to find a likely location, then search likely 
dnl directory paths for the files that you need.  Allow an environment
dnl variable to provide the values, so that if they are in a strange
dnl place, the user can set the environment variable and configure will
dnl use that value (it should still check that the value is valid, i.e.,
dnl the necessary files are in the specified directory)
dnl
dnl AC_CHECK_HEADER(jni.h)
dnl if test "$ac_cv_header_jni_h" != "yes" ; then
dnl    # Try to find the java include path.  For now, just generate warning
dnl    # At ANL, places to look include /soft/com/packages/j2sdk1.4.0/include
dnl    # and /soft/com/packages/jdk-1.3.1/include (and /soft/com/jdk*)
dnl    # May also want to look in $JAVA_HOME/include
dnl    AC_MSG_WARN([cannot compile trace code])
dnl fi
dnl java_home=/soft/com/packages/j2sdk1.4.0
dnl java_arch=linux
dnl AC_SUBST(java_home)
dnl AC_SUBST(java_arch)
dnl
dnl 09/28/2010, chan, the above checks are not enough to build
dnl libTraceInput.so for rlog.  The work is currently being done
dnl mpe2/src/slog2sdk/trace_rlog.

if test -z "$ENABLE_SHLIB" ; then
    CC_SHL="$CC"
    ENABLE_SHLIB=none
fi
PAC_CC_SUBDIR_SHLIBS

dnl AC_SEARCH_LIBS(socket, socket)
dnl AC_CHECK_LIB(thread, mutex_init)
dnl AC_CHECK_HEADERS(foo.h bar.h)
dnl AC_CHECK_FUNCS(baz biff)

dnl
dnl Etags
AC_SUBST(ETAGS)
AC_SUBST(ETAGSADD)
export ETAGS
export ETAGSADD
dnl
dnl 
dnl
dnl Other autoconf variables that may be imported from the calling environment
AC_SUBST(DOCTEXT)

dnl Generate the Makefiles from Makefile.in
AC_OUTPUT_COMMANDS([chmod a+x pprintirlog])
AC_OUTPUT(Makefile pprintirlog)
