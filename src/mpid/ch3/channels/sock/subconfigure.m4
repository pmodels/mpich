[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sock
dnl MPICH_SUBCFG_BEFORE=src/mpid/ch3/util/sock

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_CONDITIONAL([BUILD_CH3_SOCK],[test "X$device_name" = "Xch3" -a "X$channel_name" = "Xsock"])
    AM_COND_IF([BUILD_CH3_SOCK],[
        AC_MSG_NOTICE([RUNNING PREREQ FOR ch3:sock])
        # this channel depends on the sock utilities
        build_mpid_common_sock=yes
        build_ch3u_sock=yes

        MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE
        MPID_CH3I_CH_HCOLL_BCOL="basesmuma,basesmuma,ptpcoll"

        # code that formerly lived in setup_args
        #
        # Variables of interest...
        #
        # $with_device - device name and arguments
        # $device_name - name of the device
        # $device_args - contains name of channel select plus an channel args
        # $channel_name - name of the channel
        # $master_top_srcdir - top-level source directory
        # $master_top_builddir - top-level build directory
        # $ac_configure_args - all arguments passed to configure
        #
        
        found_dir="no"
        for sys in $devsubsystems ; do
            if test "$sys" = "src/mpid/common/sock" ; then
               found_dir="yes"
               break
            fi
        done
        if test "$found_dir" = "no" ; then
           devsubsystems="$devsubsystems src/mpid/common/sock"
        fi
        
        # FIXME: The setup file has a weird requirement that it needs to be
        # run *before* the MPICH device (not the setup directory itself) is
        # configured, but the actual configuration of the associated directory
        # needs to be done *after* the device is configured.
        file=${master_top_srcdir}/src/mpid/common/sock/setup
        if test -f $file ; then
           echo sourcing $file
           . $file
        fi
        
        pathlist=""
        pathlist="$pathlist src/mpid/${device_name}/channels/${channel_name}/include"
        pathlist="$pathlist src/util/wrappers"
        ## TODO delete this -I junk
        ##for path in $pathlist ; do
        ##    CPPFLAGS="$CPPFLAGS -I${master_top_builddir}/${path} -I${master_top_srcdir}/${path}"
        ##done
    ])
])dnl
dnl
dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH3_SOCK],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:sock])
# code that formerly lived in configure.ac

# FIXME this must be namespaced now that we've flattened things
#
# ABIVERSION is needed when a separate sock dll library is built
if test "X$ABIVERSION" = "X" ; then
   if test "X$libmpi_so_version" != X ; then
      ABIVERSION="$libmpi_so_version"
   else
      # Note that an install of a sock-dll will fail if the ABI version is not
      # available
      AC_MSG_WARN([Unable to set the ABIVERSION])
   fi
fi
AC_SUBST(ABIVERSION)

dnl AC_CHECK_HEADER(net/if.h) fails on Solaris; extra header files needed
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
],,lac_cv_header_net_if_h=yes,lac_cv_header_net_if_h=no)

# FIXME why doesn't this use the proper machinery?
echo "checking for net/if.h... $lac_cv_header_net_if_h"

if test "$lac_cv_header_net_if_h" = "yes" ; then
    AC_DEFINE(HAVE_NET_IF_H, 1, [Define if you have the <net/if.h> header file.])
fi

AC_CHECK_HEADERS(				\
	netdb.h					\
	sys/ioctl.h				\
	sys/socket.h				\
	sys/sockio.h				\
	sys/types.h				\
	errno.h)

# netinet/in.h often requires sys/types.h first.  With AC 2.57, check_headers
# does the right thing, which is to test whether the header is found 
# by the compiler, but this can cause problems when the header needs 
# other headers.  2.57 changes the syntax (!) of check_headers to allow 
# additional headers.
AC_CACHE_CHECK([for netinet/in.h],ac_cv_header_netinet_in_h,[
AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netinet/in.h>],[int a=0;],
    ac_cv_header_netinet_in_h=yes,
    ac_cv_header_netinet_in_h=no)])
if test "$ac_cv_header_netinet_in_h" = yes ; then
    AC_DEFINE(HAVE_NETINET_IN_H,1,[Define if netinet/in.h exists])
fi

])dnl end AM_COND_IF(BUILD_CH3_SOCK,...)
])dnl end _BODY

[#] end of __file__
