##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/tree

noinst_HEADERS += libhydra/tree/hydra_tree.h

libhydra_la_SOURCES += \
	libhydra/tree/hydra_tree.c
