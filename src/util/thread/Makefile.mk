## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/util/thread

noinst_HEADERS +=                               \
    src/util/thread/mpiu_thread.h               \
    src/util/thread/mpiu_thread_single.h        \
    src/util/thread/mpiu_thread_multiple.h      \
    src/util/thread/mpiu_thread_global.h        \
    src/util/thread/mpiu_thread_pobj.h  	\
    src/util/thread/mpiu_thread_priv.h	      	\
    src/util/thread/mpiu_thread_posix.h   	\
    src/util/thread/mpiu_thread_solaris.h 	\
    src/util/thread/mpiu_thread_win.h

mpi_core_sources += \
    src/util/thread/mpiu_thread.c \
    src/util/thread/mpiu_thread_win.c \
    src/util/thread/mpiu_thread_solaris.c \
    src/util/thread/mpiu_thread_posix.c

