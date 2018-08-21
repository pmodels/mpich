## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/rmk/src

noinst_HEADERS += libhydra/rmk/src/hydra_rmk.h

libhydra_la_SOURCES += libhydra/rmk/src/hydra_rmk.c
