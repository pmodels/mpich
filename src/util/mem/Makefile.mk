## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/util/mem

noinst_HEADERS +=                               \
    src/util/mem/mpiu_strerror.h

mpi_core_sources += \
    src/util/mem/trmem.c      \
    src/util/mem/handlemem.c  \
    src/util/mem/safestr.c    \
    src/util/mem/argstr.c     \
    src/util/mem/strerror.c

