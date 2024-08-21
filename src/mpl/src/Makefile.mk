##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

noinst_HEADERS += src/timer/mpl_timer_common.h

libmpl_base_la_SOURCES += \
    src/mpl_rankmap.c \
    src/atomic/mpl_atomic.c \
    src/bt/mpl_bt.c \
    src/dbg/mpl_dbg.c \
    src/env/mpl_env.c \
    src/mem/mpl_trmem.c \
    src/msg/mpl_msg.c \
    src/sock/mpl_sock.c \
    src/sock/mpl_sockaddr.c \
    src/sock/mpl_host.c \
    src/str/mpl_str.c \
    src/str/mpl_argstr.c \
    src/str/mpl_arg_serial.c \
    src/misc/mpl_misc.c \
    src/misc/mpl_mkstemp.c \
    src/thread/mpl_thread.c \
    src/thread/mpl_thread_win.c \
    src/thread/mpl_thread_solaris.c \
    src/thread/mpl_thread_argobots.c \
    src/thread/mpl_thread_qthreads.c \
    src/thread/mpl_thread_posix.c \
    src/thread/mpl_thread_uti.c \
    src/timer/mpl_timer_clock_gettime.c \
    src/timer/mpl_timer_gcc_ia64_cycle.c \
    src/timer/mpl_timer_gethrtime.c \
    src/timer/mpl_timer_gettimeofday.c \
    src/timer/mpl_timer_linux86_cycle.c \
    src/timer/mpl_timer_ppc64_cycle.c \
    src/timer/mpl_timer_mach_absolute_time.c \
    src/shm/mpl_shm.c \
    src/shm/mpl_shm_sysv.c \
    src/shm/mpl_shm_mmap.c \
    src/shm/mpl_shm_win.c \
    src/gavl/mpl_gavl.c

include src/gpu/Makefile.mk
