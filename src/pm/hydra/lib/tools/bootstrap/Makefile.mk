##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
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
