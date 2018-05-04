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

if BUILD_SHM_POSIX

noinst_HEADERS += src/mpid/ch4/shm/posix/posix_am.h        \
                  src/mpid/ch4/shm/posix/posix_coll.h      \
                  src/mpid/ch4/shm/posix/posix_datatypes.h \
                  src/mpid/ch4/shm/posix/shm_inline.h      \
                  src/mpid/ch4/shm/posix/posix_init.h      \
                  src/mpid/ch4/shm/posix/posix_progress.h  \
                  src/mpid/ch4/shm/posix/posix_recv.h      \
                  src/mpid/ch4/shm/posix/posix_rma.h       \
                  src/mpid/ch4/shm/posix/posix_spawn.h     \
                  src/mpid/ch4/shm/posix/posix_win.h       \
                  src/mpid/ch4/shm/posix/posix_comm.h      \
                  src/mpid/ch4/shm/posix/posix_defs.h      \
                  src/mpid/ch4/shm/posix/posix_impl.h      \
                  src/mpid/ch4/shm/posix/posix_probe.h     \
                  src/mpid/ch4/shm/posix/posix_queue.h     \
                  src/mpid/ch4/shm/posix/posix_request.h   \
                  src/mpid/ch4/shm/posix/posix_send.h      \
                  src/mpid/ch4/shm/posix/posix_startall.h  \
                  src/mpid/ch4/shm/posix/posix_unimpl.h

mpi_core_sources += src/mpid/ch4/shm/posix/globals.c    \
                    src/mpid/ch4/shm/posix/barrier.c

endif
