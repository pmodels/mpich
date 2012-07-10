## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=                                          \
    src/mpid/ch3/channels/nemesis/utils/monitor/papi_defs.c

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch3/channels/nemesis/utils/monitor

noinst_HEADERS +=                                                      \
    src/mpid/ch3/channels/nemesis/utils/monitor/my_papi_defs.h \
    src/mpid/ch3/channels/nemesis/utils/monitor/rdtsc.h

