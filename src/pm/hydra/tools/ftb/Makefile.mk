## -*- Mode: Makefile; -*-
##
## (C) 2008 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/ftb

noinst_HEADERS += tools/ftb/hydt_ftb.h

if hydra_have_ftb
libhydra_la_SOURCES += tools/ftb/hydt_ftb.c
else
libhydra_la_SOURCES += tools/ftb/hydt_ftb_dummy.c
endif
