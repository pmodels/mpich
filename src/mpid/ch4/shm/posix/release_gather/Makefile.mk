##  (C) 2018 by Argonne National Laboratory.
##      See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/posix/release_gather

noinst_HEADERS += src/mpid/ch4/shm/posix/release_gather/release_gather_types.h \
                  src/mpid/ch4/shm/posix/release_gather/release_gather.h

mpi_core_sources += src/mpid/ch4/shm/posix/release_gather/release_gather.c
