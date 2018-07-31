## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/rmk/ll

noinst_HEADERS += libhydra/rmk/ll/hydra_rmk_ll.h

libhydra_la_SOURCES += libhydra/rmk/ll/ll_detect.c \
		libhydra/rmk/ll/ll_query_node_list.c
