# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/css/include -I$(top_builddir)/css/include

include css/src/Makefile.mk
include css/utils/Makefile.mk

if hydra_css_none
include css/none/Makefile.mk
endif