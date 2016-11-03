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

if BUILD_SHM_SHMAM

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/shmam/eager/include
AM_CPPFLAGS += -I$(top_builddir)/src/mpid/ch4/shm/shmam/eager/include

noinst_HEADERS += src/mpid/ch4/shm/shmam/eager/include/shmam_eager.h
noinst_HEADERS += src/mpid/ch4/shm/shmam/eager/include/shmam_eager_impl.h

include $(top_srcdir)/src/mpid/ch4/shm/shmam/eager/fbox/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/shmam/eager/stub/Makefile.mk

endif
