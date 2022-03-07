##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/demux

noinst_HEADERS += lib/tools/demux/demux.h lib/tools/demux/demux_internal.h

libhydra_la_SOURCES += lib/tools/demux/demux.c

if hydra_have_poll
libhydra_la_SOURCES += lib/tools/demux/demux_poll.c
endif

if hydra_have_select
libhydra_la_SOURCES += lib/tools/demux/demux_select.c
endif
