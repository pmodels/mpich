## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/bootstrap/include

noinst_HEADERS += tools/bootstrap/include/bsci.h

include tools/bootstrap/src/Makefile.mk
include tools/bootstrap/utils/Makefile.mk

if hydra_bss_external
include tools/bootstrap/external/Makefile.mk
endif

if hydra_bss_persist
include tools/bootstrap/persist/Makefile.mk
endif
