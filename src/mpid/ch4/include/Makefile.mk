## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/include
AM_CPPFLAGS += -I$(top_builddir)/src/mpid/ch4/include

noinst_HEADERS += src/mpid/ch4/include/netmodpre.h  \
                  src/mpid/ch4/include/shmpre.h     \
                  src/mpid/ch4/include/mpidch4.h    \
                  src/mpid/ch4/include/mpidch4r.h   \
                  src/mpid/ch4/include/mpidimpl.h   \
                  src/mpid/ch4/include/mpidpre.h    \
                  src/mpid/ch4/include/mpid_sched.h \
                  src/mpid/ch4/include/mpid_thread.h
