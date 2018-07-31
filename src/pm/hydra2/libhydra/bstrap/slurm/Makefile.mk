## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/bstrap/slurm

noinst_HEADERS +=                     \
    libhydra/bstrap/slurm/hydra_bstrap_slurm.h

libhydra_la_SOURCES += \
	libhydra/bstrap/slurm/slurm_launch.c \
	libhydra/bstrap/slurm/slurm_finalize.c
