## -*- Mode: Makefile; -*-
##
## (C) 2008 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/demux

noinst_HEADERS += tools/demux/demux.h tools/demux/demux_internal.h

libhydra_la_SOURCES += \
	tools/demux/demux.c \
	tools/demux/demux_internal.c \
	tools/demux/demux_splice.c

if hydra_have_poll
libhydra_la_SOURCES += tools/demux/demux_poll.c
endif

if hydra_have_select
libhydra_la_SOURCES += tools/demux/demux_select.c
endif
