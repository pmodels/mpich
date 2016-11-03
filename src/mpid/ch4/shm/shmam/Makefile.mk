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

noinst_HEADERS += src/mpid/ch4/shm/shmam/shmam_am.h        \
                  src/mpid/ch4/shm/shmam/shmam_coll.h      \
                  src/mpid/ch4/shm/shmam/shmam_datatypes.h \
                  src/mpid/ch4/shm/shmam/shm_direct.h      \
                  src/mpid/ch4/shm/shmam/shmam_init.h      \
                  src/mpid/ch4/shm/shmam/shmam_progress.h  \
                  src/mpid/ch4/shm/shmam/shmam_recv.h      \
                  src/mpid/ch4/shm/shmam/shmam_rma.h       \
                  src/mpid/ch4/shm/shmam/shmam_spawn.h     \
                  src/mpid/ch4/shm/shmam/shmam_win.h       \
                  src/mpid/ch4/shm/shmam/shmam_comm.h      \
                  src/mpid/ch4/shm/shmam/shmam_defs.h      \
                  src/mpid/ch4/shm/shmam/shmam_impl.h      \
                  src/mpid/ch4/shm/shmam/shmam_probe.h     \
                  src/mpid/ch4/shm/shmam/shmam_request.h   \
                  src/mpid/ch4/shm/shmam/shmam_send.h      \
                  src/mpid/ch4/shm/shmam/shmam_unimpl.h

mpi_core_sources += src/mpid/ch4/shm/shmam/globals.c    \
                    src/mpid/ch4/shm/shmam/func_table.c \
                    src/mpid/ch4/shm/shmam/shmam_eager_array.c

include $(top_srcdir)/src/mpid/ch4/shm/shmam/eager/Makefile.mk

endif
