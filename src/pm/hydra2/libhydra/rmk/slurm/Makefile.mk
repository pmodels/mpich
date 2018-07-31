## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/rmk/slurm

noinst_HEADERS += libhydra/rmk/slurm/hydra_rmk_slurm.h

libhydra_la_SOURCES += libhydra/rmk/slurm/slurm_detect.c \
		libhydra/rmk/slurm/slurm_query_node_list.c
