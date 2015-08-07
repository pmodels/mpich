### -*- Mode: Makefile; -*-
### vim: set ft=automake :
###
### (C) 2011 by Argonne National Laboratory.
###     See COPYRIGHT in top-level directory.
###
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/timers
noinst_HEADERS += src/mpid/common/timers/mpid_timers_fallback.h
