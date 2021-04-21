##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/generic/am

noinst_HEADERS += src/mpid/ch4/generic/am/mpidig_am_send.h \
                  src/mpid/ch4/generic/am/mpidig_am_recv.h \
                  src/mpid/ch4/generic/am/mpidig_am_recv_utils.h \
                  src/mpid/ch4/generic/am/mpidig_am_send_utils.h \
                  src/mpid/ch4/generic/am/mpidig_am_req_cache.h \
                  src/mpid/ch4/generic/am/mpidig_am_part.h \
                  src/mpid/ch4/generic/am/mpidig_am_part_callbacks.h \
                  src/mpid/ch4/generic/am/mpidig_am_part_utils.h \
                  src/mpid/ch4/generic/am/mpidig_am.h

mpi_core_sources += src/mpid/ch4/generic/am/mpidig_am_init.c \
                    src/mpid/ch4/generic/am/mpidig_am_comm_abort.c \
                    src/mpid/ch4/generic/am/mpidig_am_part.c \
                    src/mpid/ch4/generic/am/mpidig_am_part_callbacks.c
