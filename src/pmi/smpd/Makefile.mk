## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_PMI_SMPD

lib_lib@MPILIBNAME@_la_SOURCES += \
    src/pmi/smpd/smpd_pmi.c       \
    src/pmi/smpd/smpd_ipmi.c

noinst_HEADERS += src/pmi/smpd/ipmi.h

AM_CPPFLAGS += -I$(top_srcdir)/src/pmi/smpd

endif BUILD_PMI_SMPD

