## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

if BUILD_CH4_SHM_POSIX_EAGER_FBOX

noinst_HEADERS += src/mpid/ch4/shm/posix/eager/fbox/fbox_init.h \
                  src/mpid/ch4/shm/posix/eager/fbox/fbox_send.h \
                  src/mpid/ch4/shm/posix/eager/fbox/fbox_recv.h \
                  src/mpid/ch4/shm/posix/eager/fbox/posix_eager_inline.h

mpi_core_sources += src/mpid/ch4/shm/posix/eager/fbox/globals.c \
                    src/mpid/ch4/shm/posix/eager/fbox/func_table.c

endif
