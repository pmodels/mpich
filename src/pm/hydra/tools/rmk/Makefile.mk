# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/include -I$(top_builddir)/tools/rmk/include

include tools/rmk/src/Makefile.mk
include tools/rmk/utils/Makefile.mk

if hydra_rmk_pbs
include tools/rmk/pbs/Makefile.mk
endif

if hydra_rmk_lsf
include tools/rmk/lsf/Makefile.mk
endif
