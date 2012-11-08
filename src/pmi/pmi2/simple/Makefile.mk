## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_PMI_PMI2_SIMPLE

lib_lib@MPILIBNAME@_la_SOURCES += \
    src/pmi/pmi2/simple/simple2pmi.c     \
    src/pmi/pmi2/simple/simple_pmiutil.c

noinst_HEADERS +=                 \
    src/pmi/pmi2/simple/simple_pmiutil.h \
    src/pmi/pmi2/simple/simple2pmi.h     \
    src/pmi/pmi2/simple/pmi2compat.h

AM_CPPFLAGS += -I$(top_srcdir)/src/pmi/pmi2/simple

endif BUILD_PMI_PMI2_SIMPLE
