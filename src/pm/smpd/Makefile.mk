## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## will define $smpd_sock_src and $smpd_sock_hdrs
include $(top_srcdir)/src/pm/smpd/sock/Makefile.mk

if BUILD_PM_SMPD

other_smpd_src =                      \
    src/pm/smpd/smpd_hash.c           \
    src/pm/smpd/smpd_authenticate.c   \
    src/pm/smpd/smpd_printf.c         \
    src/pm/smpd/smpd_read_write.c     \
    src/pm/smpd/smpd_user_data.c      \
    src/pm/smpd/smpd_connect.c        \
    src/pm/smpd/smpd_get_opt.c        \
    src/pm/smpd/smpd_launch_process.c \
    src/pm/smpd/smpd_command.c        \
    src/pm/smpd/smpd_database.c       \
    src/pm/smpd/smpd_state_machine.c  \
    src/pm/smpd/smpd_handle_command.c \
    src/pm/smpd/smpd_session.c        \
    src/pm/smpd/smpd_start_mgr.c      \
    src/pm/smpd/smpd_barrier.c        \
    src/pm/smpd/smpd_do_console.c     \
    src/pm/smpd/smpd_restart.c        \
    src/pm/smpd/smpd_host_util.c      \
    src/pm/smpd/smpd_handle_spawn.c   \
    $(smpd_sock_src)

# these files appear to be unused in the old simplemake-based UNIX build system
smpd_win_src =                       \
    src/pm/smpd/smpd_job.c           \
    src/pm/smpd/smpd_mapdrive.c      \
    src/pm/smpd/smpd_register.c      \
    src/pm/smpd/smpd_ad.cpp          \
    src/pm/smpd/smpd_affinitize.c    \
    src/pm/smpd/smpd_ccp_util.c      \
    src/pm/smpd/smpd_hpc_js_rmk.cpp  \
    src/pm/smpd/smpd_hpc_js_bs.cpp   \
    src/pm/smpd/smpd_hpc_js_util.cpp

if PRIMARY_PM_SMPD

## SMPD is built in a totally disgusting way, in the sense that it links
## directly against libmpich and the other libraries that make up the core of
## MPICH2.  This is slightly easier to manage now in the flattened automake
## implementation, but the real fix is that SMPD shouldn't be using
## MPICH2-interal routines directly.  Linking against only MPL makes much more
## sense.

# this shouldn't be necessary, ideally, but the smpd PMI impl wants to
# use symbols from the smpd PM impl
lib_lib@MPILIBNAME@_la_SOURCES += $(other_smpd_src)

bin_PROGRAMS += src/pm/smpd/mpiexec src/pm/smpd/smpd
src_pm_smpd_mpiexec_SOURCES = src/pm/smpd/mpiexec.c \
			      src/pm/smpd/mp_parse_command_line.c \
			      src/pm/smpd/mp_parse_oldconfig.c \
			      src/pm/smpd/mpiexec_rsh.c
src_pm_smpd_mpiexec_LDADD = lib/lib@MPILIBNAME@.la $(external_libs)
src_pm_smpd_mpiexec_LDFLAGS = $(external_ldflags)

src_pm_smpd_smpd_SOURCES = src/pm/smpd/smpd.c \
			   src/pm/smpd/smpd_cmd_args.c \
			   src/pm/smpd/smpd_watchprocs.c
src_pm_smpd_smpd_LDADD = lib/lib@MPILIBNAME@.la $(external_libs)
src_pm_smpd_smpd_LDFLAGS = $(external_ldflags)

noinst_HEADERS += $(smpd_sock_hdrs) src/pm/smpd/sock/include/smpd_util_sock.h
# should this go into a per-target _CPPFLAGS var instead for greater safety?
AM_CPPFLAGS += -I$(top_srcdir)/src/pm/smpd \
	       -I$(top_srcdir)/src/pm/smpd/sock/include \
	       $(smpd_sock_cppflags)

else !PRIMARY_PM_SMPD
## TODO we currently do not handle this case
endif !PRIMARY_PM_SMPD

endif BUILD_PM_SMPD

