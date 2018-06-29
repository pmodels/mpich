## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
## (C) 2014 by Mellanox Technologies, Inc.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/include

noinst_HEADERS += src/mpid/ch4/shm/include/shm.h

include $(top_srcdir)/src/mpid/ch4/shm/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/stubshm/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/posix/Makefile.mk
