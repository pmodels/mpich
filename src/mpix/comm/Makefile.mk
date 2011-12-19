## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# The group_comm code does not currently have a PMPIX version.  If/when it does,
# it should be moved to the "mpi_sources" variable.
lib_lib@MPILIBNAME@_la_SOURCES +=   \
    src/mpix/comm/group_comm.c

mpi_sources +=					\
    src/mpix/comm/comm_group_failed.c		\
    src/mpix/comm/comm_remote_group_failed.c

noinst_HEADERS += src/mpi/comm/mpicomm.h
AM_CPPFLAGS += -I$(master_top_srcdir)/src/mpi/comm