##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ(2.63)

m4_include([maint/version.m4])
AC_INIT([yaksa],YAKSA_VERSION_m4)

YAKSA_VERSION=YAKSA_VERSION_m4
AC_SUBST([YAKSA_VERSION])

libyaksa_so_version="libyaksa_so_version_m4"
AC_SUBST([libyaksa_so_version])

# Produce a numeric version assuming the following format:
# Version: [MAJ].[MIN].[REV][EXT][EXT_NUMBER]
# Example: 1.0.7rc1 has
#          MAJ = 1
#          MIN = 0
#          REV = 7
#          EXT = rc
#          EXT_NUMBER = 1
#
# Converting to numeric version will convert EXT to a format number:
#          ALPHA (a) = 0
#          BETA (b)  = 1
#          RC (rc)   = 2
#          PATCH (p) = 3
# Regular releases are treated as patch 0
#
# Numeric version will have 1 digit for MAJ, 2 digits for MIN,
# 2 digits for REV, 1 digit for EXT and 2 digits for EXT_NUMBER.
if test "${YAKSA_VERSION}" = "unreleased" ; then
    YAKSA_NUMVERSION=0
else
    changequote(<<,>>)
    V1=`expr $YAKSA_VERSION : '\([0-9]*\)\.[0-9]*\.*[0-9]*[a-zA-Z]*[0-9]*'`
    V2=`expr $YAKSA_VERSION : '[0-9]*\.\([0-9]*\)\.*[0-9]*[a-zA-Z]*[0-9]*'`
    V3=`expr $YAKSA_VERSION : '[0-9]*\.[0-9]*\.*\([0-9]*\)[a-zA-Z]*[0-9]*'`
    V4=`expr $YAKSA_VERSION : '[0-9]*\.[0-9]*\.*[0-9]*\([a-zA-Z]*\)[0-9]*'`
    V5=`expr $YAKSA_VERSION : '[0-9]*\.[0-9]*\.*[0-9]*[a-zA-Z]*\([0-9]*\)'`
    changequote([,])

    if test "$V2" -le 9 ; then V2="0$V2" ; fi
    if test "$V3" = "" ; then V3="0"; fi
    if test "$V3" -le 9 ; then V3="0$V3" ; fi
    if test "$V4" = "a" ; then
        V4=0
    elif test "$V4" = "b" ; then
        V4=1
    elif test "$V4" = "rc" ; then
        V4=2
    elif test "$V4" = "" ; then
        V4=3
        V5=0
    elif test "$V4" = "p" ; then
        V4=3
    fi
    if test "$V5" -le 9 ; then V5="0$V5" ; fi

    YAKSA_NUMVERSION=`expr $V1$V2$V3$V4$V5 + 0`
fi
AC_SUBST(YAKSA_NUMVERSION)

AC_CONFIG_AUX_DIR(m4)
AC_CONFIG_MACRO_DIR(m4)


dnl ----------------------------------------------------------------------------
dnl setup autotools
dnl ----------------------------------------------------------------------------
AM_INIT_AUTOMAKE([-Wall -Wno-portability-recursive -Werror foreign 1.12.3 subdir-objects])
AM_SILENT_RULES([yes])

PAC_PUSH_FLAG([CFLAGS])
AM_PROG_AS
PAC_POP_FLAG([CFLAGS])

AM_PROG_AR

LT_INIT

AC_PROG_INSTALL


dnl ----------------------------------------------------------------------------
dnl check the compiler and standard headers
dnl ----------------------------------------------------------------------------
dnl disable AC_PROG_CC's annoying default of adding -O2 to the CFLAGS
PAC_PUSH_FLAG([CFLAGS])
AC_PROG_CC
PAC_POP_FLAG([CFLAGS])

# see if C11 is available (for atomics)
PAC_PUSH_FLAG([CFLAGS])
PAC_APPEND_FLAG([-std=c11],[CFLAGS])
PAC_C_CHECK_COMPILER_OPTION([-std=c11],[c11_available=yes],[c11_available=no])
PAC_POP_FLAG([CFLAGS])
if test "${c11_available}" = "yes" ; then
    PAC_APPEND_FLAG([-std=c11],[CFLAGS])
else
    # PGI does not support -std option, it uses -c11 for C11
    PAC_PUSH_FLAG([CFLAGS])
    PAC_APPEND_FLAG([-c11],[CFLAGS])
    PAC_C_CHECK_COMPILER_OPTION([-c11],[c11_available=yes],[c11_available=no])
    PAC_POP_FLAG([CFLAGS])
    if test "${c11_available}" = "yes" ; then
        PAC_APPEND_FLAG([-c11],[CFLAGS])
    fi
fi

PAC_CHECK_VISIBILITY
PAC_APPEND_FLAG([$YAKSA_VISIBILITY_CFLAGS],[CFLAGS])

# required pre-Automake-1.14
AM_PROG_CC_C_O

dnl ----------------------------------------------------------------------------
dnl configure options
dnl ----------------------------------------------------------------------------
# strict
PAC_ARG_STRICT

# Add options that suppress default warnings, especially those from PGI compilers (now nvc)
PAC_C_DEFAULT_OPTIONS

# embedded builds
AC_ARG_ENABLE([embedded],AS_HELP_STRING([--enable-embedded],[enables embedded builds]),,[enable_embedded=no])
AM_CONDITIONAL([EMBEDDED_BUILD], [test x${enable_embedded} = xyes])
if test x${enable_embedded} = xyes; then
    AC_DEFINE(YAKSA_EMBEDDED_BUILD, [1], [Define if yaksa is embedded])
fi

# --enable-debug
AC_ARG_ENABLE([g],AS_HELP_STRING([--enable-g],[alias for --enable-debug]),,[enable_g=no])
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[adds -g to CFLAGS]),,[enable_debug=${enable_g}])
if test "${enable_debug}" != "no" ; then
    PAC_APPEND_FLAG([-g],[CFLAGS])
    AC_DEFINE(YAKSA_DEBUG, [1], [Define if debugging is enabled])
fi

# --enable-fast
AC_ARG_ENABLE([fast],AS_HELP_STRING([--enable-fast=O<opt>],[adds -O<opt> to CFLAGS]),,[enable_fast=O2])
PAC_PUSH_FLAG(IFS)
IFS=","
for option in ${enable_fast} ; do
    case "$option" in
        O*)
            PAC_APPEND_FLAG([-${option}],[CFLAGS])
            ;;
        ndebug)
            PAC_APPEND_FLAG([-DNDEBUG],[CFLAGS])
            ;;
        all|yes)
            PAC_APPEND_FLAG([-O2],[CFLAGS])
            PAC_APPEND_FLAG([-DNDEBUG],[CFLAGS])
            ;;
        none|no)
            PAC_APPEND_FLAG([-O0],[CFLAGS])
            ;;
        *)
            # ignore unknown options
            ;;
    esac
done
PAC_POP_FLAG(IFS)


dnl ----------------------------------------------------------------------------
dnl check the environment and the availability of functions
dnl ----------------------------------------------------------------------------
PAC_C_GNU_ATTRIBUTE

# look for stdatomic
AC_CHECK_HEADERS([stdatomic.h],[have_c11_atomics=yes],[have_c11_atomics=no])
if test "${have_c11_atomics}" = "yes"; then
    AC_DEFINE([HAVE_C11_ATOMICS],[1],[Define if C11 atomics are available])
fi
AM_CONDITIONAL([HAVE_C11_ATOMICS], [test x${have_c11_atomics} = xyes])

# look for pthreads
AC_CHECK_HEADERS(pthread.h)
AC_CHECK_LIB([pthread],[pthread_key_create],have_pthreads=yes)
if test "$have_pthreads" = "yes" ; then
    PAC_PREPEND_FLAG([-lpthread],[LIBS])
else
    AC_ERROR([pthreads not found on the system])
fi

# check alignments
AC_CHECK_ALIGNOF(_Bool)
AC_CHECK_ALIGNOF(char)
AC_CHECK_ALIGNOF(signed char)
AC_CHECK_ALIGNOF(unsigned char)
AC_CHECK_ALIGNOF(wchar_t)
AC_CHECK_ALIGNOF(int8_t)
AC_CHECK_ALIGNOF(int16_t)
AC_CHECK_ALIGNOF(int32_t)
AC_CHECK_ALIGNOF(int64_t)
AC_CHECK_ALIGNOF(uint8_t)
AC_CHECK_ALIGNOF(uint16_t)
AC_CHECK_ALIGNOF(uint32_t)
AC_CHECK_ALIGNOF(uint64_t)
AC_CHECK_ALIGNOF(int)
AC_CHECK_ALIGNOF(unsigned)
AC_CHECK_ALIGNOF(short)
AC_CHECK_ALIGNOF(unsigned short)
AC_CHECK_ALIGNOF(long)
AC_CHECK_ALIGNOF(unsigned long)
AC_CHECK_ALIGNOF(long long)
AC_CHECK_ALIGNOF(unsigned long long)
AC_CHECK_ALIGNOF(int_fast8_t)
AC_CHECK_ALIGNOF(int_fast16_t)
AC_CHECK_ALIGNOF(int_fast32_t)
AC_CHECK_ALIGNOF(int_fast64_t)
AC_CHECK_ALIGNOF(uint_fast8_t)
AC_CHECK_ALIGNOF(uint_fast16_t)
AC_CHECK_ALIGNOF(uint_fast32_t)
AC_CHECK_ALIGNOF(uint_fast64_t)
AC_CHECK_ALIGNOF(int_least8_t)
AC_CHECK_ALIGNOF(int_least16_t)
AC_CHECK_ALIGNOF(int_least32_t)
AC_CHECK_ALIGNOF(int_least64_t)
AC_CHECK_ALIGNOF(uint_least8_t)
AC_CHECK_ALIGNOF(uint_least16_t)
AC_CHECK_ALIGNOF(uint_least32_t)
AC_CHECK_ALIGNOF(uint_least64_t)
AC_CHECK_ALIGNOF(byte)
AC_CHECK_ALIGNOF(intmax_t)
AC_CHECK_ALIGNOF(uintmax_t)
AC_CHECK_ALIGNOF(size_t)
AC_CHECK_ALIGNOF(intptr_t)
AC_CHECK_ALIGNOF(uintptr_t)
AC_CHECK_ALIGNOF(ptrdiff_t)
AC_CHECK_ALIGNOF(float)
AC_CHECK_ALIGNOF(double)
AC_CHECK_ALIGNOF(long double)

# backend devices
supported_backends="seq"
m4_include([src/backend/cuda/subconfigure.m4])
m4_include([src/backend/ze/subconfigure.m4])
m4_include([src/backend/hip/subconfigure.m4])


dnl ----------------------------------------------------------------------------
dnl Final setup
dnl ----------------------------------------------------------------------------


dnl ----------------------------------------------------------------------------
dnl config headers
dnl ----------------------------------------------------------------------------
AC_CONFIG_HEADERS([src/frontend/include/yaksa_config.h])
AH_TOP([
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSA_CONFIG_H_INCLUDED
#define YAKSA_CONFIG_H_INCLUDED
])
AH_BOTTOM([
#endif /* YAKSA_CONFIG_H_INCLUDED */
])

AC_CONFIG_FILES([Makefile
                 Doxyfile
                 maint/yaksa.pc
                 src/frontend/include/yaksa.h
])
AC_OUTPUT


##################################################################
# Display the status of optional modules
##################################################################

cat <<EOF

##################################################################
# Final status of compiled yaksa modules
##################################################################

Backends supported: ${supported_backends}
${backend_info}

##################################################################
# End of yaksa configure
##################################################################

EOF
