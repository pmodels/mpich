## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/rmk/lsf

noinst_HEADERS += libhydra/rmk/lsf/hydra_rmk_lsf.h

libhydra_la_SOURCES += libhydra/rmk/lsf/lsf_detect.c \
		libhydra/rmk/lsf/lsf_query_node_list.c
