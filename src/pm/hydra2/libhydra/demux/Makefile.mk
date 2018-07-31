## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/demux

noinst_HEADERS += \
	libhydra/demux/hydra_demux.h \
	libhydra/demux/hydra_demux_internal.h

libhydra_la_SOURCES += \
	libhydra/demux/hydra_demux.c \
	libhydra/demux/hydra_demux_internal.c \
	libhydra/demux/hydra_demux_splice.c

if hydra_have_poll
libhydra_la_SOURCES += libhydra/demux/hydra_demux_poll.c
endif

if hydra_have_select
libhydra_la_SOURCES += libhydra/demux/hydra_demux_select.c
endif
