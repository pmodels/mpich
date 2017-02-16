AC_PREREQ(2.62)
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

m4_include([version.m4])
AC_INIT([Hydra], MPICH_VERSION_m4)

AC_CONFIG_AUX_DIR(confdb)
AC_CONFIG_MACRO_DIR(confdb)

# needed by hwloc in embedded mode.  Must come very early to avoid
# bizarre expansion ordering warnings
AC_CANONICAL_TARGET
AC_ARG_PROGRAM

dnl must come before LT_INIT, which AC_REQUIREs AC_PROG_CC
dnl must also come before AC_USE_SYSTEM_EXTENSIONS
PAC_PROG_CC
AM_PROG_CC_C_O

# also needed by hwloc in embedded mode, must also come early for expansion
# ordering reasons
AC_USE_SYSTEM_EXTENSIONS

# Define -D_DARWIN_C_SOURCE on OS/X to ensure that hwloc will build even if we
# are building under MPICH with --enable-strict that defined _POSIX_C_SOURCE.
# Some standard Darwin headers don't build correctly under a strict posix
# environment.
AS_CASE([$host],
    [*-*-darwin*], [PAC_APPEND_FLAG([-D_DARWIN_C_SOURCE],[CPPFLAGS])]
)


AM_INIT_AUTOMAKE([-Wall -Wno-portability-recursive -Werror foreign 1.12.3 subdir-objects])

AM_PROG_AR

AC_CONFIG_HEADER(include/hydra_config.h)

# Non-verbose make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Reset link flags
PAC_RESET_LINK_FLAGS()
WRAPPER_LIBS=""
export WRAPPER_LIBS
AC_SUBST(WRAPPER_LIBS)

PAC_ARG_STRICT

# Look for perl.  The absolute path of perl is required in hydra-doxygen.cfg.
AC_PATH_PROG(PERL,perl)

# In case the documentation targets are used, find doctext and attempt to find
# the source for the doctext LaTex style files. Use "false" if doctext is not
# found
AC_PATH_PROG(DOCTEXT,doctext,false)
export DOCTEXT
AC_SUBST(DOCTEXT)

AC_C_CONST
AC_C_VOLATILE
AC_C_RESTRICT
AC_C_INLINE

AC_CHECK_SIZEOF(unsigned long)

# Bug in autoconf adds -O2 and -g by default. Make sure we only get
# the flags we want.
PAC_PUSH_ALL_FLAGS
AC_TYPE_SIZE_T
PAC_POP_ALL_FLAGS

# Libtool might be required for third-party packages
LT_INIT

AC_PROG_MKDIR_P
AC_PROG_INSTALL
PAC_ARG_CACHING
PAC_C_MACRO_VA_ARGS
PAC_C_GNU_ATTRIBUTE
PAC_CC_FUNCTION_NAME_SYMBOL

# Build information
# these values come from the m4_include above
HYDRA_VERSION="MPICH_VERSION_m4"
HYDRA_RELEASE_DATE="MPICH_RELEASE_DATE_m4"
if test -z "$HYDRA_VERSION" ; then
    AC_MSG_ERROR([Version information not found. Configuration aborted.])
fi
AC_SUBST(HYDRA_VERSION)
AC_DEFINE_UNQUOTED(HYDRA_VERSION,"$HYDRA_VERSION",[Hydra version information])
AC_DEFINE_UNQUOTED(HYDRA_RELEASE_DATE,"$HYDRA_RELEASE_DATE",[Hydra release date information])

AC_SUBST(HYDRA_PMI_PROXY)
PAC_GET_EXENAME("hydra_pmi_proxy", HYDRA_PMI_PROXY)
AC_DEFINE_UNQUOTED(HYDRA_PMI_PROXY,"$HYDRA_PMI_PROXY",[Hydra PMI proxy executable])

# These substitutions are needed in the Makefile
AC_SUBST(MPIEXEC_NAME)
AC_SUBST(MPIEXEC_HYDRA_NAME)
AC_SUBST(MPIRUN_NAME)
PAC_GET_EXENAME("mpiexec", MPIEXEC_NAME)
PAC_GET_EXENAME("mpiexec.hydra", MPIEXEC_HYDRA_NAME)
PAC_GET_EXENAME("mpirun", MPIRUN_NAME)

PAC_PUSH_ALL_FLAGS
PAC_RESET_ALL_FLAGS
if test "$CC" != "" ; then
   AC_DEFINE_UNQUOTED(HYDRA_CC,"$CC $CPPFLAGS $CFLAGS $LDFLAGS $LIBS",[C compiler])
else
   AC_DEFINE_UNQUOTED(HYDRA_CC,"",[C compiler])
fi
if test "$CXX" != "" ; then
   AC_DEFINE_UNQUOTED(HYDRA_CXX,"$CXX $CPPFLAGS $CXXFLAGS $LDFLAGS $LIBS",[C++ compiler])
else
   AC_DEFINE_UNQUOTED(HYDRA_CXX,"",[C++ compiler])
fi
if test "$F77" != "" ; then
   AC_DEFINE_UNQUOTED(HYDRA_F77,"$F77 $FFLAGS $LDFLAGS $LIBS",[Fortran 77 compiler])
else
   AC_DEFINE_UNQUOTED(HYDRA_F77,"",[Fortran 77 compiler])
fi
if test "$FC" != "" ; then
   AC_DEFINE_UNQUOTED(HYDRA_F90,"$FC $FCFLAGS $LDFLAGS $LIBS",[Fortran 90 compiler])
else
   AC_DEFINE_UNQUOTED(HYDRA_F90,"",[Fortran 90 compiler])
fi
PAC_POP_ALL_FLAGS

AC_DEFINE_UNQUOTED(HYDRA_CONFIGURE_ARGS_CLEAN,"`echo $ac_configure_args`",[Configure arguments])

# MPL
AC_ARG_VAR([MPLLIBNAME],[can be used to override the name of the MPL library (default: "mpl")])
MPLLIBNAME=${MPLLIBNAME:-"mpl"}
export MPLLIBNAME
AC_SUBST(MPLLIBNAME)
AC_ARG_WITH([mpl-prefix],
            [AS_HELP_STRING([[--with-mpl-prefix[=DIR]]],
                            [use the MPL library installed in DIR. Pass
                             "embedded" to force usage of the MPL source
                             distributed with Hydra.])],
            [],dnl action-if-given
            [with_mpl_prefix=embedded]) dnl action-if-not-given
mpl_srcdir=""
AC_SUBST([mpl_srcdir])
# Controls whether we recurse into the MPL dir when running "dist" rules like
# "make distclean".  Technically we are cheating whenever DIST_SUBDIRS is not a
# superset of SUBDIRS, but we don't want to double-distclean and similar.
AC_SUBST(mpl_dist_srcdir)
mpl_includedir=""
AC_SUBST([mpl_includedir])
mpl_libdir=""
AC_SUBST([mpl_libdir])
mpl_lib=""
AC_SUBST([mpl_lib])
if test "$with_mpl_prefix" = "embedded" ; then
    mpl_srcdir="mpl"
    mpl_dist_srcdir="mpl"
    mpl_lib="mpl/lib${MPLLIBNAME}.la"
    mpl_subdir_args="--disable-versioning --enable-embedded"
    PAC_CONFIG_SUBDIR_ARGS([mpl],[$mpl_subdir_args],[],[AC_MSG_ERROR(MPL configure failed)])
    mpl_includedir='-I$(top_builddir)/mpl/include -I$(top_srcdir)/mpl/include'
else
    # The user specified an already-installed MPL; just sanity check, don't
    # subconfigure it
    AS_IF([test -s "${with_mpl_prefix}/include/mplconfig.h"],
          [:],[AC_MSG_ERROR([the MPL installation in "${with_mpl_prefix}" appears broken])])
    mpl_includedir="-I${with_mpl_prefix}/include"
    mpl_libdir="-L${with_mpl_prefix}/lib"
    mpl_lib="-l${MPLLIBNAME}"
fi

# Documentation
AC_PATH_PROG([DOXYGEN],[doxygen],,$PATH)
AC_SUBST(DOXYGEN)
AC_SUBST(top_srcdir)

# support gcov test coverage information
PAC_ENABLE_COVERAGE

# Check if the necessary headers are available
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h strings.h sys/types.h sys/socket.h sched.h sys/stat.h sys/param.h \
		 netinet/in.h netinet/tcp.h sys/un.h sys/time.h time.h ifaddrs.h arpa/inet.h \
		 poll.h fcntl.h netdb.h winsock2.h windows.h)

# These functions are in additional libraries on some platforms (like Solaris).
# Check to see if they are needed.
AC_SEARCH_LIBS(socket,socket,)
AC_SEARCH_LIBS(gethostbyname,nsl,)
AC_SEARCH_LIBS(hstrerror, resolv,)

# Check for necessary functions
AC_CHECK_FUNCS(gettimeofday time sigaction signal usleep alloca unsetenv \
	       strsignal stat fcntl alarm isatty inet_ntop getpgid \
	       setsid killpg hstrerror)

if test "$ac_cv_func_hstrerror" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <netdb.h>],hstrerror)
fi

if test "$ac_cv_func_gettimeofday" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <sys/time.h>],gettimeofday)
fi

if test "$ac_cv_func_getpgid" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <unistd.h>],getpgid)
fi

if test "$ac_cv_func_killpg" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <signal.h>],killpg)
fi

# Check for putenv
AC_CHECK_FUNCS(putenv)
if test "$ac_cv_func_putenv" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],putenv)
fi

AC_TRY_LINK([
	#include <stdio.h>
	#include <sys/types.h>
	#include <ifaddrs.h>
	],
	[return getifaddrs(NULL);],
	[AC_DEFINE([HAVE_GETIFADDRS],1,[Define to '1' if getifaddrs is present and usable])])

# Check what we need to do about the environ extern
AC_CACHE_CHECK([for environ in unistd.h],pac_cv_environ_in_unistd,
	[AC_TRY_COMPILE([#include <unistd.h>],[char **x = environ;],
	pac_cv_environ_in_unistd=yes,pac_cv_environ_in_unistd=no)])
if test "$pac_cv_environ_in_unistd" = "yes" ; then
    # We have environ defined in unistd.h; we are done
    AC_DEFINE(HAVE_EXTERN_ENVIRON,1,[Define if environ extern is available])
else
    # See if we can declare it
    AC_CACHE_CHECK([for manually declared extern environ], pac_cv_manual_extern_environ,
   	[AC_TRY_LINK([#include <unistd.h>],[extern char ** environ; char **x = environ;],
	pac_cv_manual_extern_environ=yes,pac_cv_manual_extern_environ=no)])
    if test "$pac_cv_manual_extern_environ" = "yes" ; then
        # We can manually declare the extern
        AC_DEFINE(MANUAL_EXTERN_ENVIRON,1,[Define if environ decl needed])
        AC_DEFINE(HAVE_EXTERN_ENVIRON,1,[Define if environ extern is available])
    fi
fi

#########################################################################
# Check what bootstrap server we should use
#########################################################################
AC_ARG_WITH(hydra-bss,
	[AC_HELP_STRING([--with-hydra-bss=name],
		[Bootstrap Server (user,ssh,rsh,fork,slurm,ll,lsf,sge,pbs,cobalt)])],
	[ hydra_bss=$withval ],
	[ hydra_bss="ssh,rsh,fork,slurm,ll,lsf,sge,pbs" ])
AC_MSG_CHECKING(bootstrap server)
AC_MSG_RESULT($hydra_bss)
hydra_bss_names="`echo $hydra_bss | sed -e 's/:/ /g' -e 's/,/ /g'`"

hydra_bss_external=false

for hydra_bss_name in ${hydra_bss_names}; do
    case "$hydra_bss_name" in
    	ssh)
		hydra_bss_external=true
		available_launchers="$available_launchers ssh"
		;;
    	rsh)
		hydra_bss_external=true
		available_launchers="$available_launchers rsh"
		;;
    	fork)
		hydra_bss_external=true
		available_launchers="$available_launchers fork"
		;;
	slurm)
		hydra_bss_external=true
		available_launchers="$available_launchers slurm"
		;;
	ll)
		hydra_bss_external=true
		available_launchers="$available_launchers ll"
		;;
	lsf)
		hydra_bss_external=true
		available_launchers="$available_launchers lsf"
		;;
	sge)
		hydra_bss_external=true
		available_launchers="$available_launchers sge"
		;;
	pbs)
		hydra_bss_external=true
		# Check if TM library support is available
		PAC_SET_HEADER_LIB_PATH(pbs)
		PAC_PUSH_FLAG(LIBS)
		PAC_CHECK_HEADER_LIB(tm.h, torque, tm_init, have_pbs_launcher=yes,
					   have_pbs_launcher=no)
		PAC_POP_FLAG(LIBS)
                if test "$have_pbs_launcher" = "yes" ; then
		    available_launchers="$available_launchers pbs"
		    PAC_APPEND_FLAG([-ltorque],[WRAPPER_LIBS])
                fi
		;;
	*)
		;;
    esac
done

## Bootstrap servers
if test "$hydra_bss_external" ; then
   AM_CONDITIONAL([hydra_bss_external], [true])
   AC_DEFINE(HAVE_BSS_EXTERNAL,1,[Define if external bss is enabled])
fi

## Launchers
if test "$have_pbs_launcher" = "yes" ; then
   AC_DEFINE(HAVE_TM_H, 1, [Define if tm.h and library are usable.])
fi
AM_CONDITIONAL([hydra_pbs_launcher], [test "$have_pbs_launcher" = "yes"])

tmp_list=
for hydra_launcher in ${available_launchers} ; do
    hydra_launcher_names="${hydra_launcher_names} \"${hydra_launcher}\", "
    if test -z "${hydra_default_launcher}" ; then
       hydra_default_launcher=$hydra_launcher
       AC_DEFINE_UNQUOTED(HYDRA_DEFAULT_LAUNCHER,"$hydra_default_launcher",[Default launcher])
       tmp_list="${hydra_launcher}"
    else
       tmp_list="${tmp_list} ${hydra_launcher}"
    fi
    hydra_launcher_init_decl="${hydra_launcher_init_decl}
HYD_status HYDT_bsci_launcher_${hydra_launcher}_init(void);"
    hydra_launcher_init_fns="${hydra_launcher_init_fns} HYDT_bsci_launcher_${hydra_launcher}_init, "
done
available_launchers=$tmp_list
hydra_launcher_names="${hydra_launcher_names} NULL"
hydra_launcher_init_fns="${hydra_launcher_init_fns} NULL"
AM_SUBST_NOTMAKE(hydra_launcher_init_decl)
AC_SUBST(hydra_launcher_names)
AC_SUBST(hydra_launcher_init_fns)
AC_SUBST(hydra_launcher_init_decl)
AC_DEFINE_UNQUOTED(HYDRA_AVAILABLE_LAUNCHERS,"${available_launchers}",
	[Definition of enabled launchers])


#########################################################################
# Check what RMK we should use
#########################################################################
AC_ARG_WITH(hydra-rmk,
	[AC_HELP_STRING([--with-hydra-rmk=name],
		[RMK (slurm,ll,lsf,sge,pbs,cobalt)])],
	[ hydra_rmk=$withval ],
	[ hydra_rmk="slurm,ll,lsf,sge,pbs,cobalt" ])
AC_MSG_CHECKING(rmk)
AC_MSG_RESULT($hydra_rmk)
hydra_rmk_list="`echo $hydra_rmk | sed -e 's/:/ /g' -e 's/,/ /g'`"

hydra_rmk_names=
hydra_rmk_names_unquoted=
hydra_rmk_detect_fns=
hydra_rmk_query_node_list_fns=
for rmk in ${hydra_rmk_list} ; do
    hydra_rmk_names="${hydra_rmk_names} \"${rmk}\", "
    if test -z "${hydra_rmk_names_unquoted}" ; then
       hydra_rmk_names_unquoted="${rmk}"
    else
       hydra_rmk_names_unquoted="${hydra_rmk_names_unquoted} ${rmk}"
    fi
    hydra_rmk_detect_fns="${hydra_rmk_detect_fns} HYDT_rmki_${rmk}_detect, "
    hydra_rmk_query_node_list_fns="${hydra_rmk_query_node_list_fns} HYDT_rmki_${rmk}_query_node_list, "
done
hydra_rmk_names="${hydra_rmk_names} NULL"
hydra_rmk_detect_fns="${hydra_rmk_detect_fns} NULL"
hydra_rmk_query_node_list_fns="${hydra_rmk_query_node_list_fns} NULL"
AC_SUBST(hydra_rmk_names)
AC_SUBST(hydra_rmk_detect_fns)
AC_SUBST(hydra_rmk_query_node_list_fns)
AC_DEFINE_UNQUOTED(HYDRA_AVAILABLE_RMKS,"${hydra_rmk_names_unquoted}",[Definition of enabled RMKS])



#########################################################################
# Check what process manager we should use
#########################################################################
AC_ARG_WITH(hydra-pm, [  --with-hydra-pm=name - Process Manager (pmiserv)],
		[ hydra_pm=$withval ],
		[ hydra_pm=pmiserv ])
AC_MSG_CHECKING(process manager)
AC_MSG_RESULT($hydra_pm)
AC_SUBST(hydra_pm)
AM_CONDITIONAL([hydra_pm_pmiserv], [test $hydra_pm = "pmiserv"])


#########################################################################
# Check what UI we should use
#########################################################################
AC_ARG_WITH(hydra-ui, [  --with-hydra-ui=name - User Interface (mpich)],
		[ hydra_ui=$withval ],
		[ hydra_ui=mpich ])
AC_MSG_CHECKING(user interface)
AC_MSG_RESULT($hydra_ui)
AC_SUBST(hydra_ui)
AM_CONDITIONAL([hydra_ui_mpich], [test $hydra_ui = "mpich"])


#########################################################################
# System hwloc
#########################################################################
PAC_CHECK_PREFIX(hwloc)


#########################################################################
# Processor Topology
#########################################################################
AC_ARG_WITH(hydra-topolib, [  --with-hydra-topolib=name - Processor Topology Library (hwloc)],
			   [ hydra_topolib_list=$withval ],
			   [ hydra_topolib_list=hwloc ])
AC_MSG_CHECKING([desired processor topology libraries])
AC_MSG_RESULT([$hydra_topolib_list])

hydra_topolibs="`echo $hydra_topolib_list | sed -e 's/:/ /g' -e 's/,/ /g'`"

# Unconditionally include the hwloc macros, even if we are using an
# external hwloc (or hwloc is disabled). This is needed for the
# AM_CONDITIONALS that we will set later.
m4_include([tools/topo/hwloc/hwloc/config/hwloc.m4])
m4_include([tools/topo/hwloc/hwloc/config/hwloc_check_attributes.m4])
m4_include([tools/topo/hwloc/hwloc/config/hwloc_check_visibility.m4])
m4_include([tools/topo/hwloc/hwloc/config/hwloc_check_vendor.m4])
m4_include([tools/topo/hwloc/hwloc/config/hwloc_components.m4])
m4_include([tools/topo/hwloc/hwloc/config/hwloc_pkg.m4])

have_hwloc=no
for hydra_topolib in ${hydra_topolibs}; do
    case "$hydra_topolib" in
	hwloc)
		if test "$with_hwloc_prefix" = "embedded" ; then
		   HWLOC_SETUP_CORE([tools/topo/hwloc/hwloc],[have_hwloc=yes],[have_hwloc=no],[1])
		   # Only build hwloc in embedded mode
		   if test "$have_hwloc" = "yes" ; then
		      hydra_use_embedded_hwloc=yes
		      CFLAGS="$HWLOC_EMBEDDED_CFLAGS $CFLAGS"
		      CPPFLAGS="$HWLOC_EMBEDDED_CPPFLAGS $CPPFLAGS"
		      LIBS="$HWLOC_EMBEDDED_LIBS $LIBS"
		   fi
                else
                    AC_CHECK_HEADERS([hwloc.h])
                    # hwloc_topology_set_pid was added in hwloc-1.0.0, which is our minimum
                    # required version
                    AC_CHECK_LIB([hwloc],[hwloc_topology_set_pid])
                    AC_MSG_CHECKING([if non-embedded hwloc works])
                    if test "$ac_cv_header_hwloc_h" = "yes" -a "$ac_cv_lib_hwloc_hwloc_topology_set_pid" = "yes" ; then
                        have_hwloc=yes
                    fi
                    AC_MSG_RESULT([$have_hwloc])
                fi

                # FIXME: Disable hwloc on Cygwin for now. The hwloc package, atleast as of 1.0.2,
                # does not install correctly on Cygwin
                AS_CASE([$host], [*-*-cygwin], [have_hwloc=no])

		if test "$have_hwloc" = "yes" ; then
		   AC_DEFINE(HAVE_HWLOC,1,[Define if hwloc is available])
		   available_topolibs="$available_topolibs hwloc"
		fi
		;;
	*)
		;;
    esac
done

tmp_list=
for hydra_topolib in ${available_topolibs}; do
    if test -z "$hydra_default_topolib" ; then
       hydra_default_topolib=${hydra_topolib}
       tmp_list="${hydra_topolib}"
    else
       tmp_list="$tmp_list ${hydra_topolib}"
    fi
done
available_topolibs=$tmp_list

AC_DEFINE_UNQUOTED(HYDRA_AVAILABLE_TOPOLIBS,"$available_topolibs",
	[Definition of enabled processor topology libraries])
if test "$hydra_default_topolib" != "" ; then
   AC_DEFINE_UNQUOTED(HYDRA_DEFAULT_TOPOLIB,"$hydra_default_topolib",
	[Default processor topology library])
else
   AC_DEFINE(HYDRA_DEFAULT_TOPOLIB,NULL,[Default processor topology library])
fi

HWLOC_DO_AM_CONDITIONALS
AM_CONDITIONAL([hydra_have_hwloc], [test "${have_hwloc}" = "yes"])
AM_CONDITIONAL([hydra_use_embedded_hwloc], [test "${hydra_use_embedded_hwloc}" = "yes"])

AC_MSG_CHECKING([available processor topology libraries])
AC_MSG_RESULT([$available_topolibs])


#########################################################################
# Demux engine
#########################################################################
AC_ARG_WITH(hydra-demux, [  --with-hydra-demux=name - Demux engine (poll, select, port)],
			 [ hydra_demux_list=$withval ],
			 [ hydra_demux_list=poll,select,port ])
AC_MSG_CHECKING(demux engine)
AC_MSG_RESULT($hydra_demux_list)

hydra_demuxes="`echo $hydra_demux_list | sed -e 's/:/ /g' -e 's/,/ /g'`"

have_poll=no
have_select=no
have_port=no
for hydra_demux in ${hydra_demuxes}; do
    case "$hydra_demux" in
    	poll)
		AC_CHECK_FUNCS(poll,[have_poll=yes],[have_poll=no])
		;;
	select)
		AC_CHECK_FUNCS(select,[have_select=yes],[have_select=no])
		;;
	port)
		# FIXME: Need to add a test for completion ports
		;;
	*)
		;;
    esac
done

AM_CONDITIONAL([hydra_have_poll], [test "${have_poll}" = "yes"])
AM_CONDITIONAL([hydra_have_select], [test "${have_select}" = "yes"])
AM_CONDITIONAL([hydra_have_port], [test "${have_port}" = "yes"])


#########################################################################
# Debugging support
#########################################################################
AC_ARG_ENABLE(g,
	[--enable-g=option - Control the level of debugging support. option is a list of comma
			   separated names including
	none     - No debugging
	dbg      - Add compiler flag -g
	mem      - Memory usage tracing
	all      - All of the above choices],,enable_g=none)
AC_MSG_CHECKING(debugging support)
AC_MSG_RESULT($enable_g)
for option in $enable_g ; do
    case $option in
        dbg|debug)
		PAC_APPEND_FLAG(-g, CFLAGS)
		;;
        mem)
		AC_DEFINE(USE_MEMORY_TRACING,1,[Define if memory tracing is enabled])
		;;
        all)
		PAC_APPEND_FLAG(-g, CFLAGS)
		AC_DEFINE(USE_MEMORY_TRACING,1,[Define if memory tracing is enabled])
		AC_DEFINE(PMI_KEY_CHECK,1,[Define if we should check for PMI key collisions])
		;;
        *)
		;;
    esac
done


#########################################################################
# Error checking
#########################################################################
AC_ARG_ENABLE(error-checking, [  --enable-error-checking - Enable error checking],
		 [ error_check=$enableval ],
		 [ error_check=yes ])
AC_MSG_CHECKING(error checking)
AC_MSG_RESULT($error_check)
if test "$error_check" = "yes"; then
   CFLAGS="$CFLAGS -g"
   AC_DEFINE(HAVE_ERROR_CHECKING,1,[Define if error checking is enabled])
fi


#########################################################################
# Warning messages
#########################################################################
AC_ARG_ENABLE(warnings, [  --enable-warnings              Enable warnings],
			[ WARNINGS=$enableval ],
			[ WARNINGS=no ])
AC_MSG_CHECKING(warnings)
AC_MSG_RESULT($WARNINGS)
if test "$WARNINGS" = "yes"; then
   AC_DEFINE(ENABLE_WARNINGS,1,[Define if warnings are enabled])
fi


#########################################################################
# Profiling
#########################################################################
AC_ARG_ENABLE(profiling, [  --enable-profiling - Enable profiling],
	[ enable_profiling=$enableval ], [ enable_profiling=yes ])
AC_MSG_CHECKING(profiling)
AC_MSG_RESULT($enable_profiling)
if test "$enable_profiling" = "yes"; then
   AC_DEFINE(ENABLE_PROFILING,1,[Define if profiling enabled])
fi


#########################################################################
# Signals and friends
#########################################################################
AC_CHECK_FUNCS(strsignal)
if test "$ac_cv_func_strsignal" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <string.h>],strsignal)
fi

AC_CHECK_FUNCS(sigaction signal sigset)
sigaction_ok=no
if test "$ac_cv_func_sigaction" = "yes" ; then
    # Make sure that the fields that we need in sigaction are defined
    AC_CACHE_CHECK([for struct sigaction and sa_handler],
    pac_cv_struct_sigaction_with_sa_handler,[
    AC_TRY_COMPILE([#include <signal.h>],[
struct sigaction act; sigaddset( &act.sa_mask, SIGINT );
act.sa_handler = SIG_IGN;],
    pac_cv_struct_sigaction_with_sa_handler=yes,
    pac_cv_struct_sigaction_with_sa_handler=no)])
    if test "$pac_cv_struct_sigaction_with_sa_handler" = "no" ; then
        AC_CACHE_CHECK([for struct sigaction and sa_handler with _POSIX_SOURCE],
	pac_cv_struct_sigaction_with_sa_handler_needs_posix,[
        AC_TRY_COMPILE([#define _POSIX_SOURCE
#include <signal.h>],[
struct sigaction act; sigaddset( &act.sa_mask, SIGINT );
act.sa_handler = SIG_IGN;],
	pac_cv_struct_sigaction_with_sa_handler_needs_posix=yes,
	pac_cv_struct_sigaction_with_sa_handler_needs_posix=no)])
        if test "$pac_cv_struct_sigaction_with_sa_handler_needs_posix" = "yes" ; then
            sigaction_ok=yes
	fi
    else
        sigaction_ok=yes
    fi
fi

# Decide on the signal handler to use
if test "$ac_cv_func_sigaction" = "yes" -a "$sigaction_ok" = "yes" ; then
    if test "$pac_cv_struct_sigaction_with_sa_handler_needs_posix" = yes ; then
        AC_DEFINE(NEEDS_POSIX_FOR_SIGACTION,1,[Define if _POSIX_SOURCE needed to get sigaction])
    fi
    AC_DEFINE(USE_SIGACTION,,[Define if sigaction should be used to set signals])
elif test "$ac_cv_func_signal" = "yes" ; then
    AC_DEFINE(USE_SIGNAL,,[Define if signal should be used to set signals])
fi

# gethostname() returns host name
AC_CHECK_FUNCS(gethostname)
if test "$ac_cv_func_gethostname" = "yes" ; then
    # Do we need to declare gethostname?
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
fi

# Final output
AC_OUTPUT(Makefile
	tools/rmk/src/rmk.c
	tools/bootstrap/src/bsci_init.c
	hydra-doxygen.cfg
)
