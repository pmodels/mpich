## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/bstrap/sge

noinst_HEADERS +=                     \
    libhydra/bstrap/sge/hydra_bstrap_sge.h

libhydra_la_SOURCES += \
	libhydra/bstrap/sge/sge_launch.c \
	libhydra/bstrap/sge/sge_finalize.c
