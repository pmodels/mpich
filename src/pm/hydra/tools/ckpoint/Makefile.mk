## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/ckpoint

noinst_HEADERS += tools/ckpoint/ckpoint.h

libhydra_la_SOURCES += tools/ckpoint/ckpoint.c

if hydra_have_blcr
include tools/ckpoint/blcr/Makefile.mk
endif
