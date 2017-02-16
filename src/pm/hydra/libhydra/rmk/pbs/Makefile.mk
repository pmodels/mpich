## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/rmk/pbs

noinst_HEADERS += libhydra/rmk/pbs/hydra_rmk_pbs.h

libhydra_la_SOURCES += libhydra/rmk/pbs/pbs_detect.c \
		libhydra/rmk/pbs/pbs_query_node_list.c
