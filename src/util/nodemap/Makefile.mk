## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## FIXME check that this is the right way to do VPATH
AM_CPPFLAGS += -I$(top_srcdir)/src/util/nodemap

noinst_HEADERS +=                             \
    src/util/nodemap/build_nodemap.h
