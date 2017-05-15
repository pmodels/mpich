## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/bstrap/ll

noinst_HEADERS +=                     \
    libhydra/bstrap/ll/hydra_bstrap_ll.h

libhydra_la_SOURCES += \
	libhydra/bstrap/ll/ll_launch.c \
	libhydra/bstrap/ll/ll_finalize.c
