## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/slurm

noinst_HEADERS += tools/rmk/slurm/slurm_rmk.h

libhydra_la_SOURCES += tools/rmk/slurm/slurm_detect.c \
		tools/rmk/slurm/slurm_query_node_list.c
