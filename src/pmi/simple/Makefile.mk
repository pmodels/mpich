## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_PMI_SIMPLE

mpi_core_sources +=       \
    src/pmi/simple/simple_pmiutil.c \
    src/pmi/simple/simple_pmi.c

noinst_HEADERS +=                   \
    src/pmi/simple/simple_pmiutil.h \
    src/pmi/include/pmi.h

AM_CPPFLAGS += -I$(top_srcdir)/src/pmi/simple
AM_CPPFLAGS += -I$(top_srcdir)/src/pmi/include

endif BUILD_PMI_SIMPLE

