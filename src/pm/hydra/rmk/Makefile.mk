# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/rmk/include -I$(top_builddir)/rmk/include

include rmk/src/Makefile.mk
include rmk/utils/Makefile.mk

if hydra_rmk_dummy
include rmk/dummy/Makefile.mk
endif

if hydra_rmk_pbs
include rmk/pbs/Makefile.mk
endif
