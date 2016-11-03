## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2014 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

if BUILD_SHM_SHMAM

noinst_HEADERS += src/mpid/ch4/shm/shmam/eager/fbox/fbox_init.h \
                  src/mpid/ch4/shm/shmam/eager/fbox/fbox_send.h \
                  src/mpid/ch4/shm/shmam/eager/fbox/fbox_recv.h

mpi_core_sources += src/mpid/ch4/shm/shmam/eager/fbox/globals.c \
                    src/mpid/ch4/shm/shmam/eager/fbox/func_table.c

endif
