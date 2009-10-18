# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/bind

libhydra_a_SOURCES += $(top_srcdir)/tools/bind/bind.c

if hydra_have_plpa
include tools/bind/plpa/Makefile.mk
endif

if hydra_have_hwloc
include tools/bind/hwloc/Makefile.mk
endif
