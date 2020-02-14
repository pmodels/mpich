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

if BUILD_CH4_SHM_POSIX_LMT_RNDV

mpi_core_sources += src/mpid/ch4/shm/posix/lmt/rndv/rndv_init.c \
                    src/mpid/ch4/shm/posix/lmt/rndv/rndv_send.c \
                    src/mpid/ch4/shm/posix/lmt/rndv/rndv_recv.c \
                    src/mpid/ch4/shm/posix/lmt/rndv/rndv_progress.c \
                    src/mpid/ch4/shm/posix/lmt/rndv/rndv_control.c
endif
