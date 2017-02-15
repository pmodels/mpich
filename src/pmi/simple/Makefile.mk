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
    src/pmi/simple/simple_pmiutil.h

AM_CPPFLAGS += -I$(top_srcdir)/src/pmi/simple

lib_LTLIBRARIES += libpmi.la
libpmi_la_SOURCES = src/pmi/simple/simple_pmiutil.c src/pmi/simple/simple_pmi.c
libpmi_la_CPPFLAGS = -DEXTERNAL_PMI_LIBRARY -I$(top_srcdir)/src/include
libpmi_la_LIBADD = @mpllib@

endif BUILD_PMI_SIMPLE

