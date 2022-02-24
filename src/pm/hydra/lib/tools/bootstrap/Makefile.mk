##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/bootstrap/include

noinst_HEADERS += lib/tools/bootstrap/include/bsci.h

include lib/tools/bootstrap/src/Makefile.mk
include lib/tools/bootstrap/utils/Makefile.mk

if hydra_bss_external
include lib/tools/bootstrap/external/Makefile.mk
endif

if hydra_bss_persist
include lib/tools/bootstrap/persist/Makefile.mk
endif
