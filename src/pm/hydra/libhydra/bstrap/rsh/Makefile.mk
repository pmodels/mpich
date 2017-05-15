## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/bstrap/rsh

noinst_HEADERS +=                     \
    libhydra/bstrap/rsh/hydra_bstrap_rsh.h

libhydra_la_SOURCES += \
	libhydra/bstrap/rsh/rsh_launch.c \
	libhydra/bstrap/rsh/rsh_finalize.c
