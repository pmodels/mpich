##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

lib@MPLLIBNAME@_la_SOURCES +=        \
    src/thread/mpl_thread.c        \
    src/thread/mpl_thread_win.c    \
    src/thread/mpl_thread_solaris.c    \
    src/thread/mpl_thread_argobots.c    \
    src/thread/mpl_thread_qthreads.c    \
    src/thread/mpl_thread_posix.c \
    src/thread/mpl_thread_uti.c
