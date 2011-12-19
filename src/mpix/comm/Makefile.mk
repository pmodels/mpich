## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=					\
    src/mpix/comm/comm_group_failed.c		\
    src/mpix/comm/comm_remote_group_failed.c

noinst_HEADERS += src/mpi/comm/mpicomm.h
AM_CPPFLAGS += -I$(master_top_srcdir)/src/mpi/comm