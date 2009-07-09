# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/utils/bind

libhydra_a_SOURCES += $(top_srcdir)/utils/bind/bind.c

if hydra_have_plpa
include utils/bind/plpa/Makefile.mk
endif
