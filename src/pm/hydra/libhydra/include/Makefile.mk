## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/include

noinst_HEADERS += libhydra/include/hydra.h \
	libhydra/include/hydra_base.h
