# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

zm_sources =
include $(top_srcdir)/src/mem/Makefile.mk
include $(top_srcdir)/src/lock/Makefile.mk
include $(top_srcdir)/src/list/Makefile.mk
include $(top_srcdir)/src/queue/Makefile.mk

include $(top_srcdir)/src/include/Makefile.mk

if ZM_EMBEDDED_MODE
noinst_LTLIBRARIES = libzm.la
else
lib_LTLIBRARIES = libzm.la
endif
libzm_la_SOURCES = $(zm_sources)
libzm_la_CPPFLAGS = -I$(top_srcdir)/src/include
libzm_la_LDFLAGS = -version-info @libzm_so_version@
