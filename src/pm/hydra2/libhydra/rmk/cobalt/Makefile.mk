## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/rmk/cobalt

noinst_HEADERS += libhydra/rmk/cobalt/hydra_rmk_cobalt.h

libhydra_la_SOURCES += libhydra/rmk/cobalt/cobalt_detect.c \
		libhydra/rmk/cobalt/cobalt_query_node_list.c
