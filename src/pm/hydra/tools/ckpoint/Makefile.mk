# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/ckpoint

libhydra_la_SOURCES += $(top_srcdir)/tools/ckpoint/ckpoint.c

if hydra_have_blcr
include tools/ckpoint/blcr/Makefile.mk
endif
