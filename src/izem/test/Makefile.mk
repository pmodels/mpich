# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS = $(DEPS_CPPFLAGS)
AM_CPPFLAGS += -I$(top_srcdir)/src/include
AM_LDFLAGS = $(DEPS_LDFLAGS)

libzm = $(top_builddir)/src/libzm.la

$(libzm):
	$(MAKE) -C $(top_builddir)/src

LDADD = $(libzm)
