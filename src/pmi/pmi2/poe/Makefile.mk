## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_PMI_PMI2_POE

mpi_core_sources += \
    src/pmi/pmi2/poe/poe2pmi.c

AM_CPPFLAGS += -I$(top_srcdir)/src/pmi/pmi2/poe

endif BUILD_PMI_PMI2_POE
