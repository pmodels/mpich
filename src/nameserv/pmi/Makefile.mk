## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NAMEPUB_PMI

mpi_core_sources +=   \
    src/nameserv/pmi/pmi_nameserv.c

endif BUILD_NAMEPUB_PMI

