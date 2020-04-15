##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_PMI_BGQ

mpi_core_sources +=       \
    src/pmi/bgq/bgq_pmi.c

noinst_HEADERS +=

AM_CPPFLAGS += -I$(top_srcdir)/src/pmi/bgq

endif BUILD_PMI_BGQ
