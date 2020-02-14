## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2019 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/posix/lmt/include
AM_CPPFLAGS += -I$(top_builddir)/src/mpid/ch4/shm/posix/lmt/include

noinst_HEADERS += src/mpid/ch4/shm/posix/lmt/include/posix_lmt.h \
                  src/mpid/ch4/shm/posix/lmt/include/posix_lmt_pre.h

include $(top_srcdir)/src/mpid/ch4/shm/posix/lmt/rndv/Makefile.mk

