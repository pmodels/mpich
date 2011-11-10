## -*- Mode: Makefile; -*-
##
## (C) 2008 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/ui/include

noinst_HEADERS += ui/include/ui.h

include ui/utils/Makefile.mk

if hydra_ui_mpich
include ui/mpich/Makefile.mk
endif
