##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/posix/release_gather

noinst_HEADERS += src/mpid/ch4/shm/posix/release_gather/release_gather_types.h \
                  src/mpid/ch4/shm/posix/release_gather/release_gather.h \
                  src/mpid/ch4/shm/posix/release_gather/nb_bcast_release_gather.h \
                  src/mpid/ch4/shm/posix/release_gather/nb_reduce_release_gather.h

mpi_core_sources += src/mpid/ch4/shm/posix/release_gather/release_gather.c \
                    src/mpid/ch4/shm/posix/release_gather/nb_release_gather.c
