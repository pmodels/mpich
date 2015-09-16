AC_PREREQ(2.63)
#
# (C) 2006 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AC_INIT([MPL], [0.1])

# sanity check that --srcdir was specified correctly
AC_CONFIG_SRCDIR([src/mplstr.c])

AC_CONFIG_AUX_DIR(confdb)
AC_CONFIG_MACRO_DIR(confdb)
AM_INIT_AUTOMAKE([subdir-objects] [-Wall -Werror foreign 1.12.3])

dnl must come before LT_INIT, which AC_REQUIREs AC_PROG_CC
PAC_PROG_CC
AM_PROG_CC_C_O

AM_PROG_AR

LT_PREREQ([2.2.6])

# Bug in libtool adds -O2 and -g by default
PAC_PUSH_FLAG([CFLAGS])
LT_INIT()
PAC_POP_FLAG([CFLAGS])

# ----------------------------------------------------------------------------
# Set default library names if names haven't already been provided
AC_ARG_VAR([MPLLIBNAME],[can be used to override the name of the MPL library (default: "mpl")])
MPLLIBNAME=${MPLLIBNAME:-"mpl"}
AC_SUBST(MPLLIBNAME)
export MPLLIBNAME

if test -s "$srcdir/VERSION" ; then
   . $srcdir/VERSION
   AC_SUBST(libmpl_so_version)
else
   AC_MSG_ERROR([Version information not found. Configuration aborted.])
fi

AC_CONFIG_HEADER([include/config.h])
AX_PREFIX_CONFIG_H([include/mplconfig.h],[MPL])

# Non-verbose make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_C_CONST
AC_C_RESTRICT
AC_C_INLINE

PAC_C_MACRO_VA_ARGS

# TODO: could add a --enable-embedded or similar to also control this
AC_ARG_VAR([FROM_MPICH],[set to "yes" if building MPL inside of MPICH])
AM_CONDITIONAL([MPL_EMBEDDED_MODE],[test "x${FROM_MPICH}" = "xyes"])

# support gcov test coverage information
PAC_ENABLE_COVERAGE

# check for compiler support for the __typeof() extension
AC_CACHE_CHECK([whether the compiler supports __typeof(variable)],
               [pac_cv_have___typeof],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]],[[double foo = 0.0; __typeof(foo) bar = 1.0;]])],
                  [pac_cv_have___typeof=yes],
                  [pac_cv_have___typeof=no])]
)
if test "$pac_cv_have___typeof" = "yes" ; then
    AC_DEFINE([HAVE___TYPEOF],[1],[defined if the C compiler supports __typeof(variable)])
fi

dnl Check if the necessary headers are available
AC_CHECK_HEADERS(stdio.h stdlib.h string.h stdarg.h ctype.h search.h sys/types.h sys/uio.h)

# A C99 compliant compiler should have inttypes.h for fixed-size int types
AC_CHECK_HEADERS(inttypes.h stdint.h)

#######################################################################
# valgrind support
AC_ARG_WITH([valgrind],
[AS_HELP_STRING([--without-valgrind],[to disable valgrind support (such as because of version issues)])]
[AS_HELP_STRING([--with-valgrind=PATH],[use valgrind headers installed in PATH (default is "yes", use no special path)])],
[],[with_valgrind=yes])
if test "$with_valgrind" != "no" ; then
    savedCPPFLAGS="$CPPFLAGS"
    if test "$with_valgrind" != "yes" ; then
        # Clients of MPL will either need to respect the localdefs file (as in
        # MPICH) or add this entry to their own CPPFLAGS-equivalent.
        # (TODO: a pkg-config file would help with this)
        PAC_APPEND_FLAG([-I${with_valgrind}], [CPPFLAGS])
    fi
    # headers for valgrind client requests
    AC_CHECK_HEADERS([valgrind.h memcheck.h valgrind/valgrind.h valgrind/memcheck.h])
    # headers for valgrind-based thread checking tools
    # TODO: incorporate ThreadSanitizer as well (include dynamic_annotations.h,
    # link with dynamic_annotations.c)
    AC_CHECK_HEADERS([helgrind.h valgrind/helgrind.h])
    AC_CHECK_HEADERS([drd.h valgrind/drd.h])

    # ensure that we have a new enough valgrind with all the client macros
    # a preproc test would probably be sufficient, but the LINK_IFELSE helps us
    # double-check that we aren't accidentally grabbing the headers for some
    # other platform
    AC_CACHE_CHECK([whether the valgrind headers are broken or too old],
                   [pac_cv_have_broken_valgrind],
                   [AC_LINK_IFELSE(
                       [AC_LANG_PROGRAM([
#if defined(HAVE_VALGRIND_H) && defined(HAVE_MEMCHECK_H)
#  include <valgrind.h>
#  include <memcheck.h>
#elif defined(HAVE_VALGRIND_VALGRIND_H) && defined(HAVE_VALGRIND_MEMCHECK_H)
#  include <valgrind/valgrind.h>
#  include <valgrind/memcheck.h>
#else
#  error unexpected valgrind header error
#endif
int foo = 10;
char mempool_obj;
],[
#if defined(VALGRIND_MAKE_MEM_DEFINED)
    VALGRIND_MAKE_MEM_NOACCESS(&foo,sizeof(foo));
    VALGRIND_MAKE_MEM_UNDEFINED(&foo,sizeof(foo));
    VALGRIND_MAKE_MEM_DEFINED(&foo,sizeof(foo));
    VALGRIND_CHECK_MEM_IS_DEFINED(&foo,sizeof(foo));
    VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&foo,sizeof(foo));
#elif defined(VALGRIND_MAKE_READABLE)
/* older (pre-3.2.0), but still supported style */
    VALGRIND_MAKE_READABLE(&foo,sizeof(foo));
    VALGRIND_MAKE_NOACCESS(&foo,sizeof(foo));
    VALGRIND_MAKE_UNDEFINED(&foo,sizeof(foo));
    VALGRIND_CHECK_READABLE(&foo,sizeof(foo));
    VALGRIND_CHECK_WRITEABLE(&foo,sizeof(foo));
#else
#error missing essential valgrind client macros
#endif
    VALGRIND_CREATE_BLOCK(&foo,sizeof(foo),"description");
    if (RUNNING_ON_VALGRIND) ++foo;
    VALGRIND_PRINTF_BACKTRACE("testing: %s","valgrind support");
    VALGRIND_CREATE_MEMPOOL(&mempool_obj,0,0);
    VALGRIND_MEMPOOL_ALLOC(&mempool_obj,&foo,sizeof(foo));
    VALGRIND_MEMPOOL_FREE(&mempool_obj,&foo);
    VALGRIND_DESTROY_MEMPOOL(&mempool_obj);
]) dnl end PROGRAM
                       ],
                       [pac_cv_have_broken_valgrind=no], dnl end if-true
                       [pac_cv_have_broken_valgrind=yes] dnl end if-false
                   )] dnl end IFELSE
                   ) dnl end CACHE_CHECK

    if test "$pac_cv_have_broken_valgrind" = "yes" ; then
        AC_DEFINE([HAVE_BROKEN_VALGRIND],[1],[define if valgrind is old and/or broken compared to what we are expecting])
        CPPFLAGS="$savedCPPFLAGS"
    fi
fi

## Enable creation of libtool-style versioning or no versioning
AC_ARG_ENABLE(versioning,
        [AC_HELP_STRING([--enable-versioning],[Enable library versioning])],,
        [enable_versioning=yes])

if test "$enable_versioning" = "yes" ; then
   libmpl_so_versionflags="-version-info \$(libmpl_so_version)"
else
   libmpl_so_versionflags="-avoid-version"
fi
export libmpl_so_versionflags
AC_SUBST(libmpl_so_versionflags)

#######################################################################


# Check for strdup
AC_CHECK_FUNCS(strdup)
if test "$ac_cv_func_strdup" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <string.h>],strdup)
fi

# Check for snprintf
AC_CHECK_FUNCS(snprintf)
if test "$ac_cv_func_snprintf" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>],snprintf)
fi

# Check for strncmp
AC_CHECK_FUNCS(strncmp)
if test "$ac_cv_func_strncmp" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <string.h>],strncmp)
fi

# Check for putenv
AC_CHECK_FUNCS(putenv)
if test "$ac_cv_func_putenv" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],putenv)
fi

# Check for strerror
AC_CHECK_FUNCS(strerror)
if test "$ac_cv_func_strerror" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <string.h>],strerror)
fi

dnl Check for ATTRIBUTE
PAC_C_GNU_ATTRIBUTE

dnl Final output
AC_CONFIG_FILES([Makefile localdefs])
AC_OUTPUT
