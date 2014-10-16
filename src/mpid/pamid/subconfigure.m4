[#] start of __file__
dnl begin_generated_IBM_copyright_prolog                             
dnl                                                                  
dnl This is an automatically generated copyright prolog.             
dnl After initializing,  DO NOT MODIFY OR MOVE                       
dnl  --------------------------------------------------------------- 
dnl Licensed Materials - Property of IBM                             
dnl Blue Gene/Q 5765-PER 5765-PRP                                    
dnl                                                                  
dnl (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           
dnl US Government Users Restricted Rights -                          
dnl Use, duplication, or disclosure restricted                       
dnl by GSA ADP Schedule Contract with IBM Corp.                      
dnl                                                                  
dnl  --------------------------------------------------------------- 
dnl                                                                  
dnl end_generated_IBM_copyright_prolog                               
dnl -*- mode: makefile-gmake; -*-

dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_PAMID],[test "$device_name" = "pamid"])
AM_CONDITIONAL([QUEUE_BINARY_SEARCH_SUPPORT],[test "$enable_queue_binary_search" = "yes"])

AC_ARG_VAR([PAMILIBNAME],[can be used to override the name of the PAMI library (default: "pami")])
original_PAMILIBNAME=${PAMILIBNAME}
PAMILIBNAME=${PAMILIBNAME:-"pami"}
AC_SUBST(PAMILIBNAME)
export PAMILIBNAME

dnl this subconfigure.m4 handles the configure work for the ftb subdir too
dnl this AM_CONDITIONAL only works because enable_ftb is set very early on by
dnl autoconf's argument parsing code.  The "action-if-given" from the
dnl AC_ARG_ENABLE has not yet run
dnl AM_CONDITIONAL([BUILD_CH3_UTIL_FTB],[test "x$enable_ftb" = "xyes"])

AM_COND_IF([BUILD_PAMID],[

pamid_platform=${device_args}
if test x"$pamid_platform" == "x"; then
  AS_CASE([$host],
        [*-bgq-*],[pamid_platform=BGQ])
fi

AC_MSG_NOTICE([Using the pamid platform '$pamid_platform'])


dnl Set a value for the maximum processor name.
MPID_MAX_PROCESSOR_NAME=128
PM_REQUIRES_PMI=pmi2
if test "${pamid_platform}" = "PE" ; then
  with_shared_memory=sysv
        PM_REQUIRES_PMI=pmi2/poe
elif test "${pamid_platform}" = "BGQ" ; then
  with_shared_memory=mmap
  MPID_DEFAULT_CROSS_FILE=${master_top_srcdir}/src/mpid/pamid/cross/bgq8
  MPID_DEFAULT_PM=no
fi

MPID_DEVICE_TIMER_TYPE=double
MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE

dnl the PAMID device depends on the common NBC scheduler code
build_mpid_common_sched=yes
build_mpid_common_datatype=yes
build_mpid_common_thread=yes


])dnl end AM_COND_IF(BUILD_PAMID,...)
])dnl end PREREQ
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_PAMID],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR PAMI DEVICE])


ASSERT_LEVEL=2
AC_ARG_WITH(assert-level,
  AS_HELP_STRING([--with-assert-level={0 1 2}],[pamid build assert-level (default: 2)]),
  [ ASSERT_LEVEL=$withval ])
AC_SUBST(ASSERT_LEVEL)
AC_DEFINE_UNQUOTED([ASSERT_LEVEL], $ASSERT_LEVEL, [The pamid assert level])

dnl
dnl This macro adds the -I to CPPFLAGS and/or the -L to LDFLAGS
dnl
PAC_SET_HEADER_LIB_PATH(pami)

dnl
dnl Set the pamid platform define.
dnl
PAC_APPEND_FLAG([-D__${pamid_platform}__], [CPPFLAGS])

dnl Check for available shared memory functions
PAC_ARG_SHARED_MEMORY
if test "$with_shared_memory" != "mmap" -a "$with_shared_memory" != "sysv"; then
    AC_MSG_ERROR([cannot support shared memory:  need either sysv shared memory functions or mmap in order to support shared memory])
fi

dnl
dnl The default is to enable the use of the recv queue binary search
dnl ... except on BGQ
dnl
enable_queue_binary_search=yes
if test "${pamid_platform}" = "BGQ" ; then
  enable_queue_binary_search=no
fi

AM_CONDITIONAL([QUEUE_BINARY_SEARCH_SUPPORT],[test "$enable_queue_binary_search" = "yes"])

dnl
dnl This configure option allows "sandbox" bgq system software to be used.
dnl
AC_ARG_WITH(bgq-install-dir,
  AS_HELP_STRING([--with-bgq-install-dir=PATH],[specify path where bgq system software can be found;
                                                may also be specified with the 'BGQ_INSTALL_DIR'
                                                environment variable]),
  [ BGQ_INSTALL_DIR=$withval ])
AC_SUBST(BGQ_INSTALL_DIR)

dnl
dnl Add bgq-specific build options.
dnl
if test "${pamid_platform}" = "BGQ" ; then

  AC_MSG_CHECKING([for BGQ system software directory])

  dnl
  dnl Specify the default bgq system software paths
  dnl
  bgq_driver_search_path="${BGQ_INSTALL_DIR} "
  for bgq_version in `echo 4 3 2 1`; do
    for bgq_release in `echo 4 3 2 1`; do
      for bgq_mod in `echo 4 3 2 1 0`; do
        bgq_driver_search_path+="/bgsys/drivers/V${bgq_version}R${bgq_release}M${bgq_mod}/ppc64 "
      done
    done
  done

  dnl Look for a bgq driver to use.
  for bgq_driver in $bgq_driver_search_path ; do
    if test -d ${bgq_driver}/spi/include ; then

      found_bgq_driver=yes

      PAC_APPEND_FLAG([-I${bgq_driver}],                        [CPPFLAGS])
      PAC_APPEND_FLAG([-I${bgq_driver}/spi/include/kernel/cnk], [CPPFLAGS])
      PAC_APPEND_FLAG([-L${bgq_driver}/spi/lib],                [LDFLAGS])
      PAC_APPEND_FLAG([-L${bgq_driver}/spi/lib],                [WRAPPER_LDFLAGS])

      dnl If the '--with-pami' and the '--with-pami-include' configure options
      dnl were NOT specified then test for a V1R2M1+ comm include directory.
      dnl
      AS_IF([test "x${with_pami_include}" = "x" ],
        [AS_IF([test "x${with_pami}" = "x" ],
          [AS_IF([test -d ${bgq_driver}/comm/include ],
            [PAC_APPEND_FLAG([-I${bgq_driver}/comm/include],    [CPPFLAGS])],
            [PAC_APPEND_FLAG([-I${bgq_driver}/comm/sys/include],[CPPFLAGS])])])])

      dnl If the '--with-pami' and the '--with-pami-lib' configure options were
      dnl NOT specified then
      dnl
      dnl   if a custom pami library name was NOT specified then test for a
      dnl   V1R2M1+ pami lib in a V1R2M1+ comm lib directory then test for a
      dnl   pre-V1R2M1 pami lib in a pre-V1R2M1 comm lib directory; otherwise
      dnl
      dnl   if a custom pami library name WAS specified then test for a custom
      dnl   pami lib in a V1R2M1+ comm lib directory then test for a custom
      dnl   pami lib in a pre-V1R2M1 comm lib directory
      dnl
      AS_IF([test "x${with_pami_lib}" = "x" ],
        [AS_IF([test "x${with_pami}" = "x" ],
          [AS_IF([test "x${original_PAMILIBNAME}" = "x" ],
            [AS_IF([test -f ${bgq_driver}/comm/lib/libpami-gcc.a ],
              [PAMILIBNAME=pami-gcc
               PAC_APPEND_FLAG([-L${bgq_driver}/comm/lib],      [LDFLAGS])
               PAC_APPEND_FLAG([-L${bgq_driver}/comm/lib],      [WRAPPER_LDFLAGS])],
              [AS_IF([test -f ${bgq_driver}/comm/sys/lib/libpami.a ],
                [PAC_APPEND_FLAG([-L${bgq_driver}/comm/sys/lib],[LDFLAGS])
                 PAC_APPEND_FLAG([-L${bgq_driver}/comm/sys/lib],[WRAPPER_LDFLAGS])])])],
            [AS_IF([test -f ${bgq_driver}/comm/lib/lib${PAMILIBNAME}.a ],
              [PAC_APPEND_FLAG([-L${bgq_driver}/comm/lib],      [LDFLAGS])
               PAC_APPEND_FLAG([-L${bgq_driver}/comm/lib],      [WRAPPER_LDFLAGS])],
              [AS_IF([test -f ${bgq_driver}/comm/sys/lib/lib${PAMILIBNAME}.a ],
                [PAC_APPEND_FLAG([-L${bgq_driver}/comm/sys/lib],[LDFLAGS])
                 PAC_APPEND_FLAG([-L${bgq_driver}/comm/sys/lib],[WRAPPER_LDFLAGS])])])])])])

      break
    fi
  done

  if test "x${found_bgq_driver}" = "xyes"; then
    AC_MSG_RESULT('$bgq_driver')
  else
    AC_MSG_RESULT('no')
  fi

  dnl
  dnl The bgq compile requires these libraries.
  dnl
  PAC_APPEND_FLAG([-lSPI],            [LIBS])
  PAC_APPEND_FLAG([-lSPI_cnk],        [LIBS])
  PAC_APPEND_FLAG([-lrt],             [LIBS])
  PAC_APPEND_FLAG([-lpthread],        [LIBS])
  PAC_APPEND_FLAG([-lstdc++],         [LIBS])

  dnl
  dnl The wrapper scripts require these libraries.
  dnl
  PAC_APPEND_FLAG([-l${PAMILIBNAME}], [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lSPI],            [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lSPI_cnk],        [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lrt],             [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lpthread],        [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lstdc++],         [WRAPPER_LIBS])


  AC_SEARCH_LIBS([PAMI_Send], [${PAMILIBNAME} pami-gcc])


  dnl For some reason, on bgq, libtool will incorrectly attempt a static link
  dnl of libstdc++.so unless this '-all-static' option is used. This seems to
  dnl be a problem specific to libstdc++.
  dnl
  dnl Only the 'cpi', 'mpivars', and 'mpichversion' executables have this problem.
  MPID_LIBTOOL_STATIC_FLAG="-all-static"

  dnl Another bgq special case. The default linker behavior is to use static versions
  dnl of libraries. This makes supporting interlibrary dependencies difficult. Just
  dnl disable them to make our lives easier.
  if test "$INTERLIB_DEPS" = "yes"; then
	INTERLIB_DEPS="no"
  fi
fi

if test "${pamid_platform}" = "PE" ; then
        MPID_MAX_ERROR_STRING=512
fi

PAC_APPEND_FLAG([-I${master_top_srcdir}/src/include],              [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/util/wrappers],        [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/pamid/include],   [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/datatype], [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/locks],    [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/thread],   [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/sched],    [CPPFLAGS])

dnl
dnl Check for PAMI_IN_PLACE
dnl
AC_MSG_CHECKING([for PAMI_IN_PLACE support])
have_pami_in_place=0
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include "pami.h"]],
                   [[void * foo = PAMI_IN_PLACE;]])],
  have_pami_in_place=1
)
if test "$have_pami_in_place" != "0"; then
  AC_DEFINE(HAVE_PAMI_IN_PLACE,1,[Define if PAMI_IN_PLACE is defined in pami.h])
  AC_MSG_RESULT('yes')
else
  AC_DEFINE(PAMI_IN_PLACE,((void *) -1L),[Define if PAMI_IN_PLACE is not defined in pami.h])
  AC_MSG_RESULT('no')
fi

dnl
dnl Check for PAMI_CLIENT_NONCONTIG
dnl
AC_MSG_CHECKING([for PAMI_CLIENT_NONCONTIG support])
have_pami_client_noncontig=0
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include "pami.h"]],
                   [[int foo = PAMI_CLIENT_NONCONTIG;]])],
  have_pami_client_noncontig=1
)
if test "$have_pami_client_noncontig" != "0"; then
  AC_DEFINE(HAVE_PAMI_CLIENT_NONCONTIG,1,[Define if PAMI_CLIENT_NONCONTIG is defined in pami.h])
  AC_MSG_RESULT('yes')
else
  AC_MSG_RESULT('no')
fi

dnl
dnl Check for PAMI_CLIENT_MEMORY_OPTIMIZE
dnl
AC_MSG_CHECKING([for PAMI_CLIENT_MEMORY_OPTIMIZE support])
have_pami_client_memory_optimize=0
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include "pami.h"]],
                   [[int foo = PAMI_CLIENT_MEMORY_OPTIMIZE;]])],
  have_pami_client_memory_optimize=1
)
if test "$have_pami_client_memory_optimize" != "0"; then
  AC_DEFINE(HAVE_PAMI_CLIENT_MEMORY_OPTIMIZE,1,[Define if PAMI_CLIENT_MEMORY_OPTIMIZE is defined in pami.h])
  AC_MSG_RESULT('yes')
else
  AC_MSG_RESULT('no')
fi

dnl
dnl Check for PAMI_GEOMETRY_NONCONTIG
dnl
AC_MSG_CHECKING([for PAMI_GEOMETRY_NONCONTIG support])
have_pami_geometry_noncontig=0
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include "pami.h"]],
                   [[int foo = PAMI_GEOMETRY_NONCONTIG;]])],
  have_pami_geometry_noncontig=1
)
if test "$have_pami_geometry_noncontig" != "0"; then
  AC_DEFINE(HAVE_PAMI_GEOMETRY_NONCONTIG,1,[Define if PAMI_GEOMETRY_NONCONTIG is defined in pami.h])
  AC_MSG_RESULT('yes')
else
  AC_MSG_RESULT('no')
fi

dnl
dnl Check for PAMI_GEOMETRY_MEMORY_OPTIMIZE
dnl
AC_MSG_CHECKING([for PAMI_GEOMETRY_MEMORY_OPTIMIZE support])
have_pami_geometry_memory_optimize=0
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include "pami.h"]],
                   [[int foo = PAMI_GEOMETRY_MEMORY_OPTIMIZE;]])],
  have_pami_geometry_memory_optimize=1
)
if test "$have_pami_geometry_memory_optimize" != "0"; then
  AC_DEFINE(HAVE_PAMI_GEOMETRY_MEMORY_OPTIMIZE,1,[Define if PAMI_GEOMETRY_MEMORY_OPTIMIZE is defined in pami.h])
  AC_MSG_RESULT('yes')
else
  AC_MSG_RESULT('no')
fi










])dnl end AM_COND_IF(BUILD_PAMID,...)
])dnl end _BODY

[#] end of __file__
