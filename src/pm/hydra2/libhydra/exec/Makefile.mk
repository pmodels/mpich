##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/exec

noinst_HEADERS += libhydra/exec/hydra_exec.h

libhydra_la_SOURCES += \
	libhydra/exec/hydra_exec.c
