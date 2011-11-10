## -*- Mode: Makefile; -*-
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                          \
    src/mpi/spawn/comm_disconnect.c     \
    src/mpi/spawn/comm_get_parent.c     \
    src/mpi/spawn/comm_join.c           \
    src/mpi/spawn/comm_spawn.c          \
    src/mpi/spawn/comm_spawn_multiple.c \
    src/mpi/spawn/lookup_name.c         \
    src/mpi/spawn/publish_name.c        \
    src/mpi/spawn/unpublish_name.c      \
    src/mpi/spawn/open_port.c           \
    src/mpi/spawn/close_port.c          \
    src/mpi/spawn/comm_connect.c        \
    src/mpi/spawn/comm_accept.c

noinst_HEADERS += src/mpi/spawn/namepub.h

# for namepub.h, which is included by some other dirs
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/spawn

