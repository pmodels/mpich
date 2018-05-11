## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_MPID_COMMON_BC

# there are no AC_OUTPUT_FILES headers, so builddir is unnecessary
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/bc

noinst_HEADERS += src/mpid/common/bc/mpidu_bc.h

mpi_core_sources += src/mpid/common/bc/mpidu_bc.c

endif BUILD_MPID_COMMON_BC
