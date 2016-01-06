## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/util/refcount

noinst_HEADERS +=                               \
    src/util/refcount/mpir_refcount.h           \
    src/util/refcount/mpir_refcount_global.h	\
    src/util/refcount/mpir_refcount_pobj.h	\
    src/util/refcount/mpir_refcount_single.h
