# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_la_SOURCES += $(top_srcdir)/tools/bind/hwloc/bind_hwloc.c

AM_CPPFLAGS += -I$(top_srcdir)/tools/bind/hwloc/hwloc/include \
	-I$(top_builddir)/tools/bind/hwloc/hwloc/include

# Append hwloc to the external subdirs, so it gets built first
external_subdirs += tools/bind/hwloc/hwloc
external_ldflags += -L$(top_builddir)/tools/bind/hwloc/hwloc/src
external_libs += -lhwloc
