# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/demux

libhydra_a_SOURCES += $(top_srcdir)/tools/demux/demux.c

if hydra_have_poll
libhydra_a_SOURCES += $(top_srcdir)/tools/demux/demux_poll.c
endif

if hydra_have_select
libhydra_a_SOURCES += $(top_srcdir)/tools/demux/demux_select.c
endif
