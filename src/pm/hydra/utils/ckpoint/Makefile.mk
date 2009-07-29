# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/utils/ckpoint

libhydra_a_SOURCES += $(top_srcdir)/utils/ckpoint/ckpoint.c

if hydra_have_blcr
include utils/ckpoint/blcr/Makefile.mk
endif
