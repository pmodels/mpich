## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/rmk/sge

noinst_HEADERS += libhydra/rmk/sge/hydra_rmk_sge.h

libhydra_la_SOURCES += libhydra/rmk/sge/sge_detect.c \
		libhydra/rmk/sge/sge_query_node_list.c
