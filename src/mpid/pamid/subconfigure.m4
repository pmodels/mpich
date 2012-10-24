[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_PAMID],[test "$device_name" = "pamid"])

dnl this subconfigure.m4 handles the configure work for the ftb subdir too
dnl this AM_CONDITIONAL only works because enable_ftb is set very early on by
dnl autoconf's argument parsing code.  The "action-if-given" from the
dnl AC_ARG_ENABLE has not yet run
dnl AM_CONDITIONAL([BUILD_CH3_UTIL_FTB],[test "x$enable_ftb" = "xyes"])

AM_COND_IF([BUILD_PAMID],[

pamid_platform=${device_args}

# Set a value for the maximum processor name.
MPID_MAX_PROCESSOR_NAME=128

MPID_DEVICE_TIMER_TYPE=double
MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE

# the PAMID device depends on the common NBC scheduler code
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

#
# This macro adds the -I to CPPFLAGS and/or the -L to LDFLAGS
#
PAC_SET_HEADER_LIB_PATH(pami)

#
# Set the pamid platform define.
#
PAC_APPEND_FLAG([-D__${pamid_platform}__], [CPPFLAGS])

#
# This configure option allows "sandbox" bgq system software to be used.
#
AC_ARG_WITH(bgq-install-dir,
  AS_HELP_STRING([--with-bgq-install-dir=PATH],[specify path where bgq system software can be found;
                                                may also be specified with the 'BGQ_INSTALL_DIR'
                                                environment variable]),
  [ BGQ_INSTALL_DIR=$withval ])
AC_SUBST(BGQ_INSTALL_DIR)

#
# Add bgq-specific build options.
#
if test "${pamid_platform}" = "BGQ" ; then

  #
  # Specify the default bgq system software paths
  #
  bgq_driver_search_path="${BGQ_INSTALL_DIR} /bgsys/drivers/ppcfloor "
  for bgq_version in `echo 1 2 3 4`; do
    for bgq_release in `echo 1 2 3 4`; do
      for bgq_mod in `echo 0 1 2 3 4`; do
        bgq_driver_search_path+="/bgsys/drivers/V${bgq_version}R${bgq_release}M${bgq_mod}/ppc64 "
      done
    done
  done

  # Look for a bgq driver to use.
  for bgq_driver in $bgq_driver_search_path ; do
    if test -d ${bgq_driver}/spi/include ; then

      PAC_APPEND_FLAG([-I${bgq_driver}],                        [CPPFLAGS])
      PAC_APPEND_FLAG([-I${bgq_driver}/comm/sys/include],       [CPPFLAGS])
      PAC_APPEND_FLAG([-I${bgq_driver}/spi/include],            [CPPFLAGS])
      PAC_APPEND_FLAG([-I${bgq_driver}/spi/include/kernel/cnk], [CPPFLAGS])

      PAC_APPEND_FLAG([-I${bgq_driver}],                        [WRAPPER_CPPFLAGS])
      PAC_APPEND_FLAG([-I${bgq_driver}/comm/sys/include],       [WRAPPER_CPPFLAGS])
      PAC_APPEND_FLAG([-I${bgq_driver}/spi/include],            [WRAPPER_CPPFLAGS])
      PAC_APPEND_FLAG([-I${bgq_driver}/spi/include/kernel/cnk], [WRAPPER_CPPFLAGS])

      PAC_APPEND_FLAG([-L${bgq_driver}/spi/lib],                [LDFLAGS])

      PAC_APPEND_FLAG([-L${bgq_driver}/spi/lib],                [WRAPPER_LDFLAGS])
      PAC_APPEND_FLAG([-L${bgq_driver}/comm/sys/lib],           [WRAPPER_LDFLAGS])

      PAC_APPEND_FLAG([-L${bgq_driver}/spi/lib],                [WRAPPER_LDFLAGS])
      PAC_APPEND_FLAG([-L${bgq_driver}/comm/sys/lib],           [WRAPPER_LDFLAGS])

      break
    fi
  done

  #
  # The bgq compile requires these libraries.
  #
  PAC_APPEND_FLAG([-lpami],      [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lSPI],       [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lSPI_cnk],   [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lrt],        [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lpthread],   [WRAPPER_LIBS])
  PAC_APPEND_FLAG([-lstdc++],    [WRAPPER_LIBS])

  # For some reason, on bgq, libtool will incorrectly attempt a static link
  # of libstdc++.so unless this '-all-static' option is used. This seems to
  # be a problem specific to libstdc++.
  #
  # Only the 'cpi' and 'mpichversion' executables have this problem.
  MPID_LIBTOOL_STATIC_FLAG="-all-static"
fi


#
# Check for gnu-style option to enable all warnings; if specified, then
# add gnu option to treat all warnings as errors.
#
if echo $CFLAGS | grep -q -- -Wall
then
    PAC_APPEND_FLAG([-Werror],   [CFLAGS])
fi

PAC_APPEND_FLAG([-I${master_top_srcdir}/src/include],              [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/util/wrappers],        [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/pamid/include],   [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/datatype], [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/locks],    [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/thread],   [CPPFLAGS])
PAC_APPEND_FLAG([-I${master_top_srcdir}/src/mpid/common/sched],    [CPPFLAGS])

])dnl end AM_COND_IF(BUILD_PAMID,...)
])dnl end _BODY

[#] end of __file__
