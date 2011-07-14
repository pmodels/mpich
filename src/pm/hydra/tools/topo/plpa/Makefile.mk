# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_la_SOURCES += $(top_srcdir)/tools/topo/plpa/topo_plpa.c

if hydra_use_embedded_plpa
AM_CPPFLAGS += -I$(top_srcdir)/tools/topo/plpa/plpa/src/libplpa \
	-I$(top_builddir)/tools/topo/plpa/plpa/src/libplpa

# Append plpa to the external subdirs, so it gets built first
external_subdirs += tools/topo/plpa/plpa
external_ldflags += -L$(top_builddir)/tools/topo/plpa/plpa/src/libplpa
external_libs += -lplpa_included
endif
