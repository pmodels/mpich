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

